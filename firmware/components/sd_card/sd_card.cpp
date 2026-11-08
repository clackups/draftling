#include "sdkconfig.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <strings.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_heap_caps.h>
#include <soc/soc_caps.h>
#include <esp_vfs_fat.h>
#include <diskio_sdmmc.h>
#include <sdmmc_cmd.h>
#if SOC_SDMMC_HOST_SUPPORTED
#include <driver/sdmmc_host.h>
#endif
#include <driver/sdspi_host.h>
#include <driver/spi_common.h>
#include <driver/spi_master.h>
#include <driver/gpio.h>

#include "sd_card.h"

static const char *TAG = "SDCard";
static sdmmc_card_t *s_card = NULL;
static char s_mount[32] = "";
static int  s_spi_host  = -1;     /* set when mounted via SPI, otherwise -1 */
static bool s_spi_bus_owned = false;
#if defined(CONFIG_DRAFTLING_DISPLAY_EPDIY)
/* Permanent no-CS "keepalive" device kept on the shared SPI bus for the
 * lifetime of the bus on epdiy boards. See sd_card_init_spi() for the
 * rationale: it prevents the SPI master driver's shared interrupt from
 * being torn down by esp_vfs_fat_sdspi_mount()'s internal device
 * add/remove churn, which would otherwise disturb epdiy's shared LCD
 * frame interrupt. */
static spi_device_handle_t s_keepalive_dev = NULL;
#endif

/* Atomic writes (sd_card_write_file_parts(), sd_card_replace_with())
 * go through "<dir>/.<name>.dtmp" (being written) and
 * "<dir>/.<name>.dnew" (complete, about to replace <name>). After a
 * power loss a .dnew is the newest complete version of <name> and
 * replaces it -- when the old file was already removed it is the only
 * copy left. A .dtmp may be incomplete and is dropped. Looks at the
 * root, the project folders and their Git repositories (refs and HEAD
 * are written the same way), skipping .git/objects and .git/tmp. */
static void recover_dir(const char *dir, int depth, bool in_git)
{
    DIR *d = opendir(dir);
    if (!d) return;
    char *path = (char *)malloc(3 * 512);
    if (!path) { closedir(d); return; }
    char *tmp = path + 512, *sub = path + 1024;
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        const char *n = ent->d_name;
        size_t len = strlen(n);
        if (ent->d_type == DT_DIR) {
            bool into_git = depth == 1 && strcmp(n, ".git") == 0;
            bool go = (depth == 0 && n[0] != '.') || into_git ||
                      (in_git && depth < 8 && strcmp(n, ".") != 0 &&
                       strcmp(n, "..") != 0 &&
                       !(depth == 2 && (strcmp(n, "objects") == 0 ||
                                        strcmp(n, "tmp") == 0)));
            if (go) {
                snprintf(sub, 512, "%s/%s", dir, n);
                recover_dir(sub, depth + 1, in_git || into_git);
            }
            continue;
        }
        if (len < 7 || n[0] != '.') continue;
        bool is_tmp = strcmp(n + len - 5, ".dtmp") == 0;
        bool is_new = strcmp(n + len - 5, ".dnew") == 0;
        if (!is_tmp && !is_new) continue;
        snprintf(tmp, 512, "%s/%s", dir, n);
        if (is_tmp) {
            ESP_LOGW(TAG, "Dropping interrupted write %s", tmp);
            remove(tmp);
            continue;
        }
        snprintf(path, 512, "%s/%.*s", dir, (int)(len - 6), n + 1);
        remove(path);
        if (rename(tmp, path) == 0)
            ESP_LOGW(TAG, "Recovered %s from an interrupted save", path);
        else
            ESP_LOGE(TAG, "Cannot recover %s", path);
    }
    free(path);
    closedir(d);
}

static void recover_interrupted_writes(void)
{
    recover_dir(s_mount, 0, false);
}

#if SOC_SDMMC_HOST_SUPPORTED
extern "C" esp_err_t sd_card_init(int clk_pin, int cmd_pin, int d0_pin, const char *mount_point)
{
    strncpy(s_mount, mount_point, sizeof(s_mount) - 1);

    esp_vfs_fat_sdmmc_mount_config_t mount_cfg = {};
    mount_cfg.format_if_mount_failed = false;
    mount_cfg.max_files              = 10;
    mount_cfg.allocation_unit_size   = 16 * 1024;

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot = SDMMC_SLOT_CONFIG_DEFAULT();
    slot.width = 1;
    slot.clk   = (gpio_num_t)clk_pin;
    slot.cmd   = (gpio_num_t)cmd_pin;
    slot.d0    = (gpio_num_t)d0_pin;

    esp_err_t ret = esp_vfs_fat_sdmmc_mount(s_mount, &host, &slot, &mount_cfg, &s_card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Mount failed: %s", esp_err_to_name(ret));
        s_card = NULL;
        return ret;
    }
    sdmmc_card_print_info(stdout, s_card);
    ESP_LOGI(TAG, "SD card mounted at %s", s_mount);
    recover_interrupted_writes();
    return ESP_OK;
}
#else
/* Chips without an SDMMC host (ESP32-C5) mount the card over SPI only
 * (sd_card_init_spi()); no board on them selects
 * CONFIG_DRAFTLING_SD_SDMMC, so this stub only keeps the API linkable. */
extern "C" esp_err_t sd_card_init(int clk_pin, int cmd_pin, int d0_pin, const char *mount_point)
{
    (void)clk_pin; (void)cmd_pin; (void)d0_pin; (void)mount_point;
    ESP_LOGE(TAG, "SDMMC host not available on this chip");
    return ESP_ERR_NOT_SUPPORTED;
}
#endif

extern "C" esp_err_t sd_card_init_spi(int spi_host, int miso, int mosi, int sck,
                                      int cs, int enable_gpio,
                                      const char *mount_point)
{
    strncpy(s_mount, mount_point, sizeof(s_mount) - 1);
    s_spi_host = spi_host;

    /* Optional power-enable GPIO (active-high) for boards that gate
     * VBUS to the MicroSD slot through a transistor. None of the
     * currently supported boards use it (PaperS3 wires VBUS
     * directly), but the wiring is kept for forward compatibility. */
    if (enable_gpio >= 0) {
        gpio_config_t en_cfg = {};
        en_cfg.intr_type    = GPIO_INTR_DISABLE;
        en_cfg.mode         = GPIO_MODE_OUTPUT;
        en_cfg.pin_bit_mask = (1ULL << enable_gpio);
        en_cfg.pull_up_en   = GPIO_PULLUP_DISABLE;
        en_cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
        gpio_config(&en_cfg);
        gpio_set_level((gpio_num_t)enable_gpio, 1);
        /* Allow the card a moment to power up before initialization. */
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    /* Initialize the SPI bus only if the caller has not already done so
     * (e.g. the e-paper driver may share the bus). */
    if (sck >= 0 && mosi >= 0) {
        spi_bus_config_t bus_cfg = {};
        bus_cfg.mosi_io_num     = mosi;
        bus_cfg.miso_io_num     = miso;
        bus_cfg.sclk_io_num     = sck;
        bus_cfg.quadwp_io_num   = -1;
        bus_cfg.quadhd_io_num   = -1;
        bus_cfg.max_transfer_sz = 4096;
        esp_err_t ret = spi_bus_initialize((spi_host_device_t)spi_host,
                                           &bus_cfg, SPI_DMA_CH_AUTO);
        if (ret == ESP_OK) {
            s_spi_bus_owned = true;
        } else if (ret != ESP_ERR_INVALID_STATE) {
            ESP_LOGE(TAG, "SPI bus init failed: %s", esp_err_to_name(ret));
            return ret;
        }
    }

#if defined(CONFIG_DRAFTLING_DISPLAY_EPDIY)
    /* Hold a permanent no-CS keepalive device on the bus before the
     * mount loop runs.
     *
     * NOTE: this is a defensive measure, not the actual fix for the
     * no-card "epd_prep" watchdog wedge. An earlier theory blamed the
     * SPI master driver tearing its per-bus interrupt down and back up
     * as esp_vfs_fat_sdspi_mount() adds/removes its sdspi device on
     * each failed attempt, supposedly disturbing epdiy's
     * ESP_INTR_FLAG_SHARED LCD interrupt. That theory is wrong: the
     * ESP-IDF v5.5 SPI master allocates its host interrupt WITHOUT
     * ESP_INTR_FLAG_SHARED (a dedicated interrupt), so SPI device
     * add/remove cannot disturb epdiy's separately-allocated shared
     * LCD interrupt. The real cause is an EPD flush overlapping the
     * slow (down to 1 MHz) no-card mount probe; that is serialized in
     * app_main() by holding the LVGL port lock across SD init.
     *
     * The keepalive device is kept anyway because it is harmless:
     * keeping at least one device on the bus avoids needless
     * interrupt alloc/free churn during the retry ladder. It has no CS
     * line and is never used for transactions. */
    if (s_spi_bus_owned && !s_keepalive_dev) {
        spi_device_interface_config_t ka_cfg = {};
        ka_cfg.clock_speed_hz = 1 * 1000 * 1000;
        ka_cfg.mode           = 0;
        ka_cfg.spics_io_num   = -1;
        ka_cfg.queue_size     = 1;
        esp_err_t kerr = spi_bus_add_device((spi_host_device_t)spi_host,
                                            &ka_cfg, &s_keepalive_dev);
        if (kerr != ESP_OK) {
            ESP_LOGW(TAG, "SPI keepalive device add failed: %s",
                     esp_err_to_name(kerr));
            s_keepalive_dev = NULL;
        }
    }
#endif

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = spi_host;
    /* SPI data-phase clock retry ladder. We start at the SDSPI
     * spec-default 20 MHz (SDMMC_FREQ_DEFAULT) and step down on any
     * mount / post-mount probe failure.
     *
     * History: an earlier revision capped this at 4 MHz to work
     * around a 968 MB SanDisk SU01G SDSC card that mounted at 10 MHz
     * but then failed FatFs's first FAT/cluster-chain read with
     * 0x108 ESP_ERR_INVALID_RESPONSE on the LilyGO T5 E-Paper S3
     * Pro's shared SPI3 bus (the SX1262 LoRa radio sits on the same
     * bus). The 4 MHz cap fixed that card but broke every healthy
     * SDHC card -- a 16 GB SanDisk SD16G that ran fine at 20 MHz
     * started returning 0x107 ESP_ERR_TIMEOUT from
     * sdmmc_read_sectors_dma on the very first real read.
     *
     * The correct guard against the SU01G failure mode is the
     * post-mount data-phase probe below (broadened to touch the
     * FAT / cluster region a multi-GB FAT32 volume actually uses),
     * not a global clock cap. Starting at 20 MHz lets healthy SDHC
     * cards run at full speed, and the step-down ladder
     * (10 -> 4 -> 2 -> 1 MHz) catches the marginal SDSC cases. */
    static const int kRetryFreqKhz[] = {
        20000,
        10000,
        4000,
        2000,
        1000,
    };
    host.max_freq_khz = kRetryFreqKhz[0];

    sdspi_device_config_t slot = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot.gpio_cs   = (gpio_num_t)cs;
    slot.host_id   = (spi_host_device_t)spi_host;

    esp_vfs_fat_sdmmc_mount_config_t mount_cfg = {};
    mount_cfg.format_if_mount_failed = false;
    mount_cfg.max_files              = 10;
    mount_cfg.allocation_unit_size   = 16 * 1024;

    /* Some SD cards (notably older SDSC ones, and the SanDisk Edge
     * series we ship with the LilyGO T5 E-Paper S3 Pro) need a
     * surprisingly long time after VBUS comes up before they will
     * respond cleanly to CMD8 (SEND_IF_COND). On the LilyGO board
     * VBUS is wired straight to the slot (no SD_EN gating), so the
     * card is only just powered when sd_card_init_spi() runs and we
     * routinely lose the first probe with
     *   sdmmc_sd: sdmmc_init_sd_if_cond: send_if_cond (1) returned 0x108
     *   vfs_fat_sdmmc: sdmmc_card_init failed (0x108)
     * (0x108 = ESP_ERR_INVALID_RESPONSE -- the card echoed the wrong
     * check pattern back in its R7 because it was still in its
     * power-on ramp). A 100 ms settle plus a couple of mount
     * retries -- each at a lower bus clock -- gets every card we
     * have tested through reliably without slowing the happy path
     * measurably. */
    vTaskDelay(pdMS_TO_TICKS(100));

    esp_err_t ret = ESP_FAIL;
    const int attempts = sizeof(kRetryFreqKhz) / sizeof(kRetryFreqKhz[0]);
    for (int attempt = 0; attempt < attempts; ++attempt) {
        host.max_freq_khz = kRetryFreqKhz[attempt];
        ret = esp_vfs_fat_sdspi_mount(s_mount, &host, &slot,
                                      &mount_cfg, &s_card);
        if (ret == ESP_OK) {
            /* Mount uses the 400 kHz probe rate; the data-phase
             * clock (host.max_freq_khz) is only exercised when real
             * sectors are read. Old SDSC cards on the LilyGO T5
             * shared SPI bus mount cleanly at 10 MHz but then return
             * ESP_ERR_INVALID_RESPONSE / data-CRC errors from
             * sdmmc_read_sectors_dma the moment FatFs touches the
             * FAT / root-directory sectors (observed pattern:
             * corrupted start-token "6d 5f ff ff ..." in
             * sdspi_host; sector 0 alone reads cleanly because it is
             * a small DMA that just happens to land in a good
             * window). Stress the bus by reading a spread of
             * sectors that overlaps the area FatFs will hit first
             * (sector 0 BPB + a chunk likely covering the FAT and
             * root dir on a sub-1 GB FAT16 volume). If any of them
             * fail, unmount and step down. */
            /* Single-sector probes spread across the volume. The
             * higher offsets (16k, 64k, 256k) cover where FatFs's
             * first FAT / root-dir / cluster-chain reads actually
             * land on multi-GB FAT32 SDHC cards -- the original
             * narrow probe (max sector 4096) only touched the
             * reserved region and missed the area that fails. */
            const uint32_t kProbeSectors[] = {
                0, 1, 16, 64, 256, 1024, 4096,
                16384, 65536, 262144,
            };
            /* Probe buffer sized for the 4-sector burst test below.
             * Allocated on the heap (PSRAM is fine -- sdmmc handles
             * the bounce buffer when DMA-incapable memory is passed)
             * because main_task's stack is too tight to hold 2 KiB
             * of scratch; an earlier revision that put `probe` on
             * the stack caused
             * "***ERROR*** A stack overflow in task main has been
             * detected." right after sdspi_transaction probed the
             * card. */
            const size_t kProbeBufSize = 4 * 512;
            uint8_t *probe = (uint8_t *)malloc(kProbeBufSize);
            if (!probe) {
                ESP_LOGW(TAG, "SD/SPI probe buffer alloc failed -- "
                              "skipping data-phase verify at %d kHz",
                         host.max_freq_khz);
                break;
            }
            esp_err_t rerr = ESP_OK;
            for (size_t i = 0;
                 i < sizeof(kProbeSectors) / sizeof(kProbeSectors[0]);
                 ++i) {
                uint32_t sect = kProbeSectors[i];
                if (sect >= s_card->csd.capacity) continue;
                rerr = sdmmc_read_sectors(s_card, probe, sect, 1);
                if (rerr != ESP_OK) {
                    ESP_LOGW(TAG, "SD/SPI probe read sector %u at %d kHz "
                                  "failed: %s",
                             (unsigned)sect, host.max_freq_khz,
                             esp_err_to_name(rerr));
                    break;
                }
            }
            /* Multi-sector burst probe. The SU01G symptom was a
             * corrupted start-token only on burst transfers; a
             * single-sector read at the same offset would succeed.
             * A 4-sector read near sector 0 (BPB + reserved area)
             * reliably reproduces that failure mode without
             * depending on a particular FAT layout. */
            if (rerr == ESP_OK && s_card->csd.capacity >= 4) {
                rerr = sdmmc_read_sectors(s_card, probe, 0, 4);
                if (rerr != ESP_OK) {
                    ESP_LOGW(TAG, "SD/SPI 4-sector burst probe at %d kHz "
                                  "failed: %s",
                             host.max_freq_khz, esp_err_to_name(rerr));
                }
            }
            free(probe);
            if (rerr == ESP_OK) {
                if (attempt > 0) {
                    ESP_LOGW(TAG, "SD/SPI mount succeeded on attempt %d "
                                  "at %d kHz", attempt + 1, host.max_freq_khz);
                }
                break;
            }
            ESP_LOGW(TAG, "SD/SPI data-phase verify at %d kHz failed: %s "
                          "-- unmounting and stepping down",
                     host.max_freq_khz, esp_err_to_name(rerr));
            esp_vfs_fat_sdcard_unmount(s_mount, s_card);
            s_card = NULL;
            ret = rerr;
        } else {
            ESP_LOGW(TAG, "SD/SPI mount attempt %d/%d at %d kHz failed: %s",
                     attempt + 1, attempts, host.max_freq_khz,
                     esp_err_to_name(ret));
            s_card = NULL;
        }
        if (attempt + 1 < attempts) {
            /* Back off progressively (200, 400 ms) -- enough for a
             * slow card to finish its power-on initialisation
             * sequence between tries. */
            vTaskDelay(pdMS_TO_TICKS(200 * (attempt + 1)));
        }
    }
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SD/SPI mount failed: %s", esp_err_to_name(ret));
#if defined(CONFIG_DRAFTLING_DISPLAY_EPDIY)
        /* On the epdiy e-paper boards (M5Stack PaperS3, LilyGO T5
         * E-Paper S3 Pro / Pro Lite) leave the SPI bus initialized
         * after a mount failure rather than freeing it.
         *
         * NOTE: an earlier theory blamed freeing the SPI controller's
         * interrupt for disturbing epdiy's ESP_INTR_FLAG_SHARED LCD
         * interrupt and wedging the "epd_prep" feeders. That theory is
         * wrong -- the ESP-IDF v5.5 SPI master uses a dedicated,
         * non-shared interrupt, so freeing the SPI bus cannot disturb
         * the LCD interrupt. The real no-card wedge is an EPD flush
         * overlapping the slow mount probe, which app_main() now
         * serializes with the LVGL port lock around SD init.
         *
         * We still leave the bus owned here because it is harmless:
         * nothing else needs the bus freed for this session
         * (sd_card_deinit() still frees it on a clean unmount), and it
         * avoids extra bus teardown/setup churn. */
        ESP_LOGW(TAG, "Leaving SPI%d bus initialized after mount failure "
                      "to avoid disturbing the shared e-paper LCD interrupt",
                 spi_host);
#else
        if (s_spi_bus_owned) {
            spi_bus_free((spi_host_device_t)spi_host);
            s_spi_bus_owned = false;
        }
#endif
        s_card = NULL;
        return ret;
    }
    sdmmc_card_print_info(stdout, s_card);
    ESP_LOGI(TAG, "SD card mounted at %s (SPI host %d, CS=%d)",
             s_mount, spi_host, cs);
    recover_interrupted_writes();
    return ESP_OK;
}

extern "C" esp_err_t sd_card_deinit(void)
{
    if (!s_card) return ESP_ERR_INVALID_STATE;
    esp_err_t ret;
    if (s_spi_host >= 0) {
        ret = esp_vfs_fat_sdcard_unmount(s_mount, s_card);
#if defined(CONFIG_DRAFTLING_DISPLAY_EPDIY)
        /* Release the keepalive device before freeing the bus so the
         * SPI driver can shut down cleanly. */
        if (s_keepalive_dev) {
            spi_bus_remove_device(s_keepalive_dev);
            s_keepalive_dev = NULL;
        }
#endif
        if (s_spi_bus_owned) {
            spi_bus_free((spi_host_device_t)s_spi_host);
            s_spi_bus_owned = false;
        }
        s_spi_host = -1;
    } else {
        ret = esp_vfs_fat_sdcard_unmount(s_mount, s_card);
    }
    s_card = NULL;
    return ret;
}

extern "C" esp_err_t sd_card_format(void)
{
    if (!s_card) return ESP_ERR_INVALID_STATE;
    /* Same settings as the mount. esp_vfs_fat_sdcard_format_cfg()
     * unmounts the FAT volume, re-creates it and mounts it again at
     * the same path; when that re-mount fails it releases the card,
     * so forget the handle then. */
    esp_vfs_fat_mount_config_t cfg = {};
    cfg.format_if_mount_failed = false;
    cfg.max_files              = 10;
    cfg.allocation_unit_size   = 16 * 1024;
    ESP_LOGW(TAG, "Formatting the SD card at %s", s_mount);
    esp_err_t ret = esp_vfs_fat_sdcard_format_cfg(s_mount, s_card, &cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Format failed: %s", esp_err_to_name(ret));
    }
    /* Pointer lookup only: s_card has been freed if this fails. */
    if (ff_diskio_get_pdrv_card(s_card) == 0xFF) {
        s_card = NULL;
        if (ret == ESP_OK) ret = ESP_FAIL;
    }
    return ret;
}

extern "C" bool sd_card_is_ready(void)
{
    return s_card != NULL && sdmmc_get_status(s_card) == ESP_OK;
}

extern "C" sdmmc_card_t *sd_card_get_handle(void)
{
    return s_card;
}

extern "C" esp_err_t sd_card_get_fs_space(uint64_t *total_bytes, uint64_t *free_bytes)
{
    if (!s_card) return ESP_ERR_INVALID_STATE;
    return esp_vfs_fat_info(s_mount, total_bytes, free_bytes);
}

extern "C" const char *sd_card_get_mount_point(void)
{
    return s_mount;
}

extern "C" esp_err_t sd_card_read_file(const char *path, char **out_buf, size_t *out_len)
{
    if (!s_card) return ESP_ERR_INVALID_STATE;
    FILE *f = fopen(path, "rb");
    if (!f) { ESP_LOGE(TAG, "Open failed: %s", path); return ESP_ERR_NOT_FOUND; }

    long sz = -1;
    if (fseek(f, 0, SEEK_END) == 0) sz = ftell(f);
    if (sz < 0 || fseek(f, 0, SEEK_SET) != 0) { fclose(f); return ESP_FAIL; }

    char *buf = (char *)malloc(sz + 1);
    if (!buf) { fclose(f); return ESP_ERR_NO_MEM; }

    size_t rd = fread(buf, 1, sz, f);
    bool err = ferror(f);
    fclose(f);
    if (err || rd != (size_t)sz) {
        /* A partial read must not pass for the whole file (git.cfg,
         * wifi.cfg, metadata). */
        ESP_LOGE(TAG, "Read failed: %s (%zu/%ld bytes)", path, rd, sz);
        free(buf);
        return ESP_FAIL;
    }
    buf[rd] = '\0';

    *out_buf = buf;
    if (out_len) *out_len = rd;
    return ESP_OK;
}

extern "C" esp_err_t sd_card_write_file(const char *path, const char *data, size_t len)
{
    return sd_card_write_file_parts(path, data, len, NULL, 0);
}

extern "C" esp_err_t sd_card_write_file_parts(const char *path,
                                              const char *a, size_t alen,
                                              const char *b, size_t blen)
{
    if (!s_card) return ESP_ERR_INVALID_STATE;

    /* Atomic write: stream the new contents into a hidden temporary
     * next to the target (".<basename>.dtmp", skipped by the file
     * browser; rename must stay within one filesystem), flush it all
     * the way down to the card, then sd_card_replace_with() swaps it in.
     * A power loss or card yank at any point leaves the old or the new
     * contents, never a truncated file (see recover_interrupted_writes()). */
    char tmp[512];
    const char *slash = strrchr(path, '/');
    int n;
    if (slash) {
        n = snprintf(tmp, sizeof(tmp), "%.*s/.%s.dtmp",
                     (int)(slash - path), path, slash + 1);
    } else {
        n = snprintf(tmp, sizeof(tmp), ".%s.dtmp", path);
    }
    if (n <= 0 || (size_t)n >= sizeof(tmp)) {
        ESP_LOGE(TAG, "Path too long for atomic write: %s", path);
        return ESP_FAIL;
    }

    FILE *f = fopen(tmp, "wb");
    if (!f) { ESP_LOGE(TAG, "Open for write failed: %s", tmp); return ESP_FAIL; }
    size_t len = alen + blen;
    size_t wr = alen ? fwrite(a, 1, alen, f) : 0;
    if (wr == alen && blen) wr += fwrite(b, 1, blen, f);
    bool ok = (wr == len);
    if (fflush(f) != 0) ok = false;
    if (ok && fsync(fileno(f)) != 0) ok = false;
    if (fclose(f) != 0) ok = false;
    if (!ok) {
        ESP_LOGE(TAG, "Write failed: %s (%zu/%zu bytes)", tmp, wr, len);
        remove(tmp);
        return ESP_FAIL;
    }

    return sd_card_replace_with(tmp, path);
}

/* "<dir>/.<base><suffix>" for path "<dir>/<base>". */
static bool hidden_name(const char *path, const char *suffix, char *out, size_t osz)
{
    const char *slash = strrchr(path, '/');
    int n = slash ? snprintf(out, osz, "%.*s/.%s%s", (int)(slash - path), path,
                             slash + 1, suffix)
                  : snprintf(out, osz, ".%s%s", path, suffix);
    return n > 0 && (size_t)n < osz;
}

extern "C" esp_err_t sd_card_replace_with(const char *tmp, const char *path)
{
    /* FAT cannot rename over an existing file, so the old file has to
     * go first. The complete new contents are renamed to
     * ".<name>.dnew" before that: if power is lost between removing
     * the old file and the final rename, recover_interrupted_writes()
     * finds the .dnew at the next mount and finishes the job. (A
     * leftover .dtmp is always incomplete and is dropped.) */
    char done[512];
    if (!hidden_name(path, ".dnew", done, sizeof(done))) {
        ESP_LOGE(TAG, "Path too long for atomic write: %s", path);
        remove(tmp);
        return ESP_FAIL;
    }
    remove(done);
    if (rename(tmp, done) != 0) {
        ESP_LOGE(TAG, "Rename %s -> %s failed: %s", tmp, done, strerror(errno));
        remove(tmp);
        return ESP_FAIL;
    }
    remove(path);
    if (rename(done, path) != 0) {
        /* Leave .dnew in place: it is the only complete copy now, and
         * the next mount moves it back. */
        ESP_LOGE(TAG, "Rename %s -> %s failed: %s", done, path, strerror(errno));
        return ESP_FAIL;
    }
    return ESP_OK;
}

extern "C" bool sd_card_file_exists(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0;
}

extern "C" esp_err_t sd_card_delete_file(const char *path)
{
    return (remove(path) == 0) ? ESP_OK : ESP_FAIL;
}

extern "C" esp_err_t sd_card_mkdir(const char *path)
{
    /* Create parent directories as needed */
    char tmp[256];
    strncpy(tmp, path, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            mkdir(tmp, 0755);
            *p = '/';
        }
    }
    return (mkdir(tmp, 0755) == 0 || errno == EEXIST) ? ESP_OK : ESP_FAIL;
}

/* qsort() comparator: directories first, then files; both groups
 * sorted by name case-insensitively so the file browser shows a
 * predictable, alphabetical listing regardless of FAT directory
 * order. */
static int sd_entry_cmp(const void *a, const void *b)
{
    const sd_card_file_entry_t *ea = (const sd_card_file_entry_t *)a;
    const sd_card_file_entry_t *eb = (const sd_card_file_entry_t *)b;
    if (ea->is_dir != eb->is_dir) return ea->is_dir ? -1 : 1;
    return strcasecmp(ea->name, eb->name);
}

extern "C" esp_err_t sd_card_list_dir(const char *path, sd_card_file_entry_t **out,
                                      int *count)
{
    *out = NULL;
    *count = 0;
    DIR *dir = opendir(path);
    if (!dir) return ESP_FAIL;
    sd_card_file_entry_t *v = NULL;
    int n = 0, cap = 0;
    esp_err_t ret = ESP_OK;
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        if (ent->d_name[0] == '.') continue;
        if (n == cap) {
            int nc = cap ? cap * 2 : 32;
            sd_card_file_entry_t *nv = (sd_card_file_entry_t *)heap_caps_realloc(
                v, (size_t)nc * sizeof(*v), MALLOC_CAP_SPIRAM);
            if (!nv) nv = (sd_card_file_entry_t *)heap_caps_realloc(
                v, (size_t)nc * sizeof(*v), MALLOC_CAP_DEFAULT);
            if (!nv) { ret = ESP_ERR_NO_MEM; break; }
            v = nv;
            cap = nc;
        }
        sd_card_file_entry_t *e = &v[n];
        strlcpy(e->name, ent->d_name, sizeof(e->name));
        e->is_dir = (ent->d_type == DT_DIR);

        char full[512];
        snprintf(full, sizeof(full), "%s/%s", path, ent->d_name);
        struct stat st;
        e->size = (stat(full, &st) == 0) ? st.st_size : 0;
        n++;
    }
    closedir(dir);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "list_dir %s: out of memory after %d entries", path, n);
        heap_caps_free(v);
        return ret;
    }
    if (n > 1) qsort(v, (size_t)n, sizeof(v[0]), sd_entry_cmp);
    *out = v;
    *count = n;
    return ESP_OK;
}

extern "C" void sd_card_free_list(sd_card_file_entry_t *list)
{
    if (list) heap_caps_free(list);
}

extern "C" long sd_card_file_size(const char *path)
{
    struct stat st;
    return (stat(path, &st) == 0) ? st.st_size : -1;
}

extern "C" esp_err_t sd_card_rename(const char *old_path, const char *new_path)
{
    return (rename(old_path, new_path) == 0) ? ESP_OK : ESP_FAIL;
}

extern "C" bool sd_card_is_dir(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

extern "C" esp_err_t sd_card_copy_file(const char *src, const char *dst)
{
    if (!s_card) return ESP_ERR_INVALID_STATE;
    if (sd_card_file_exists(dst)) return ESP_ERR_INVALID_STATE;
    FILE *in = fopen(src, "rb");
    if (!in) return ESP_FAIL;
    FILE *out = fopen(dst, "wb");
    if (!out) { fclose(in); return ESP_FAIL; }
    const size_t CHUNK = 4096;
    char *buf = (char *)malloc(CHUNK);
    esp_err_t ret = buf ? ESP_OK : ESP_ERR_NO_MEM;
    while (ret == ESP_OK) {
        size_t n = fread(buf, 1, CHUNK, in);
        if (n == 0) { if (ferror(in)) ret = ESP_FAIL; break; }
        if (fwrite(buf, 1, n, out) != n) ret = ESP_FAIL;
    }
    free(buf);
    fclose(in);
    if (ret == ESP_OK) fsync(fileno(out));
    fclose(out);
    if (ret != ESP_OK) remove(dst);
    return ret;
}

/* Entries are unlinked while the directory is being read: FatFs only
 * marks a deleted entry free, so the read position stays valid. */
extern "C" esp_err_t sd_card_remove_tree(const char *path)
{
    DIR *dir = opendir(path);
    if (!dir) return ESP_FAIL;
    esp_err_t ret = ESP_OK;
    char *full = (char *)malloc(512);
    if (!full) { closedir(dir); return ESP_ERR_NO_MEM; }
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) continue;
        snprintf(full, 512, "%s/%s", path, ent->d_name);
        bool is_dir = (ent->d_type == DT_DIR);
        if (is_dir) {
            if (sd_card_remove_tree(full) != ESP_OK) ret = ESP_FAIL;
        } else if (remove(full) != 0) {
            ESP_LOGW(TAG, "remove_tree: cannot delete %s", full);
            ret = ESP_FAIL;
        }
    }
    closedir(dir);
    free(full);
    if (rmdir(path) != 0) {
        ESP_LOGW(TAG, "remove_tree: cannot remove %s", path);
        ret = ESP_FAIL;
    }
    return ret;
}
