/*
 * Linux: the WiFi manager.
 *
 * The network belongs to the operating system; Git sync and Draftbox
 * use it directly. What the editor calls "WiFi connected" is: the
 * computer has a network connection (NetworkManager's state is
 * "connected...", or, without NetworkManager, there is a default
 * route). The status shows the WiFi network's name, or the name of
 * the active connection (a cable, say), and the address of the
 * interface the default route uses.
 *
 * With NetworkManager running, "Choose network..." scans and connects
 * through nmcli, and Ctrl+W / "Connect to <SSID>" connects to the
 * network named in <data folder>/wifi.cfg or the one saved in the
 * settings -- as on the device, wifi.cfg wins. nmcli is the
 * NetworkManager command-line client; using it keeps D-Bus and libnm
 * out of the program. Without NetworkManager those commands report
 * that WiFi is managed by the system.
 *
 * nmcli is slow to start (tens of ms), so the state is not asked for
 * on every call: a monitor thread refreshes it every MONITOR_MS and
 * reports changes through the state callback, as the device's WiFi
 * event handler does.
 */

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "esp_log.h"
#include "nvs.h"
#include "sd_card.h"
#include "wifi_manager.h"

extern char **environ;

static const char *TAG = "WiFi";

#define WIFI_CFG_NAME "wifi.cfg"
#define MONITOR_MS 3000
#define CONNECT_TIMEOUT_S 30

static pthread_mutex_t s_mu = PTHREAD_MUTEX_INITIALIZER;     /* the fields below */
static pthread_mutex_t s_op_mu = PTHREAD_MUTEX_INITIALIZER;  /* connect / scan */
static wifi_state_t s_state = WIFI_STATE_IDLE;
static wifi_state_callback_t s_callback;
static char s_ssid[64];
static char s_ip[INET_ADDRSTRLEN];
static bool s_ipv6;
static bool s_auth_failed;
static bool s_nm;           /* NetworkManager answers */
static bool s_busy;         /* a connect is running: the monitor stays quiet */
static bool s_started;

/* ---- running nmcli ------------------------------------------------------- */

/* Run argv (argv[0] looked up in PATH) with LC_ALL=C; its standard
 * output (and, with err, standard error) goes into out (NUL-terminated,
 * cut at out_sz). Returns the exit status, or -1 when it could not be
 * run or ran past timeout_s. */
static int run(char *const argv[], char *out, size_t out_sz, bool err, int timeout_s)
{
    int pfd[2];
    if (pipe(pfd) != 0) return -1;
    fcntl(pfd[0], F_SETFD, FD_CLOEXEC);
    posix_spawn_file_actions_t fa;
    posix_spawn_file_actions_init(&fa);
    posix_spawn_file_actions_adddup2(&fa, pfd[1], STDOUT_FILENO);
    if (err) posix_spawn_file_actions_adddup2(&fa, pfd[1], STDERR_FILENO);
    else posix_spawn_file_actions_addopen(&fa, STDERR_FILENO, "/dev/null", O_WRONLY, 0);
    posix_spawn_file_actions_addopen(&fa, STDIN_FILENO, "/dev/null", O_RDONLY, 0);
    posix_spawn_file_actions_addclose(&fa, pfd[0]);

    /* The environment with LC_ALL=C, so the output can be parsed. */
    size_t n = 0;
    while (environ[n]) n++;
    char **env = (char **)calloc(n + 2, sizeof(char *));
    size_t k = 0;
    if (env) {
        for (size_t i = 0; i < n; i++)
            if (strncmp(environ[i], "LC_ALL=", 7) != 0) env[k++] = environ[i];
        env[k++] = (char *)"LC_ALL=C";
        env[k] = NULL;
    }
    pid_t pid;
    int rc = posix_spawnp(&pid, argv[0], &fa, NULL, argv, env ? env : environ);
    posix_spawn_file_actions_destroy(&fa);
    free(env);
    close(pfd[1]);
    if (rc != 0) {
        close(pfd[0]);
        return -1;
    }

    size_t len = 0;
    if (out_sz) out[0] = '\0';
    time_t until = time(NULL) + timeout_s;
    bool timed_out = false;
    for (;;) {
        struct pollfd p = { .fd = pfd[0], .events = POLLIN };
        int left = (int)(until - time(NULL));
        if (left <= 0) { timed_out = true; break; }
        int pr = poll(&p, 1, left * 1000);
        if (pr < 0 && errno == EINTR) continue;
        if (pr <= 0) { timed_out = true; break; }
        char buf[1024];
        ssize_t r = read(pfd[0], buf, sizeof(buf));
        if (r < 0 && errno == EINTR) continue;
        if (r <= 0) break;
        if (out_sz && len + 1 < out_sz) {
            size_t c = (size_t)r < out_sz - 1 - len ? (size_t)r : out_sz - 1 - len;
            memcpy(out + len, buf, c);
            len += c;
            out[len] = '\0';
        }
    }
    close(pfd[0]);
    if (timed_out) kill(pid, SIGTERM);
    int st = 0;
    while (waitpid(pid, &st, 0) < 0 && errno == EINTR) { }
    if (timed_out) return -1;
    return WIFEXITED(st) ? WEXITSTATUS(st) : -1;
}

/* nmcli -t output: fields separated by ':', a literal ':' or '\' is
 * escaped with '\'. Splits line in place into up to max fields. */
static int split_terse(char *line, char **fields, int max)
{
    int n = 0;
    char *w = line;
    fields[n++] = w;
    for (char *r = line; *r; r++) {
        if (*r == '\\' && r[1]) {
            *w++ = *++r;
        } else if (*r == ':' && n < max) {
            *w++ = '\0';
            fields[n++] = w;
        } else {
            *w++ = *r;
        }
    }
    *w = '\0';
    return n;
}

static bool nm_running(void)
{
    char out[64];
    char *argv[] = { "nmcli", "-t", "-f", "RUNNING", "general", NULL };
    return run(argv, out, sizeof(out), false, 5) == 0 && strncmp(out, "running", 7) == 0;
}

/* ---- what the system says ------------------------------------------------ */

/* The interface of the IPv4 default route (/proc/net/route). */
static bool default_route_iface(char *iface, size_t n)
{
    FILE *f = fopen("/proc/net/route", "r");
    if (!f) return false;
    char line[256];
    bool found = false;
    if (fgets(line, sizeof(line), f)) {   /* header */
        while (fgets(line, sizeof(line), f)) {
            char name[64];
            unsigned long dest, gw, flags;
            if (sscanf(line, "%63s %lx %lx %lx", name, &dest, &gw, &flags) == 4 &&
                dest == 0 && (flags & 1 /* RTF_UP */)) {
                snprintf(iface, n, "%s", name);
                found = true;
                break;
            }
        }
    }
    fclose(f);
    return found;
}

static bool has_ipv6_default_route(void)
{
    FILE *f = fopen("/proc/net/ipv6_route", "r");
    if (!f) return false;
    char line[256];
    bool found = false;
    while (fgets(line, sizeof(line), f)) {
        /* dest (32 hex) dest_len ... ; ::/0 is the default route */
        if (strncmp(line, "00000000000000000000000000000000 00 ", 36) == 0 &&
            !strstr(line, " lo\n")) {
            found = true;
            break;
        }
    }
    fclose(f);
    return found;
}

/* IPv4 address of iface, and whether any interface has a global IPv6
 * address (not link-local, unique-local or loopback). */
static void read_addresses(const char *iface, char *ip, size_t ip_sz, bool *v6)
{
    ip[0] = '\0';
    *v6 = false;
    struct ifaddrs *ifa = NULL;
    if (getifaddrs(&ifa) != 0) return;
    for (struct ifaddrs *a = ifa; a; a = a->ifa_next) {
        if (!a->ifa_addr || !(a->ifa_flags & IFF_UP) || (a->ifa_flags & IFF_LOOPBACK)) continue;
        if (a->ifa_addr->sa_family == AF_INET && iface && !ip[0] &&
            strcmp(a->ifa_name, iface) == 0) {
            inet_ntop(AF_INET, &((struct sockaddr_in *)a->ifa_addr)->sin_addr, ip, (socklen_t)ip_sz);
        } else if (a->ifa_addr->sa_family == AF_INET6) {
            const uint8_t *b = ((struct sockaddr_in6 *)a->ifa_addr)->sin6_addr.s6_addr;
            bool link_local = b[0] == 0xfe && (b[1] & 0xc0) == 0x80;
            bool ula = (b[0] & 0xfe) == 0xfc;
            if (!link_local && !ula && (b[0] & 0xe0) == 0x20) *v6 = true;
        }
    }
    freeifaddrs(ifa);
    if (*v6 && !has_ipv6_default_route()) *v6 = false;
}

/* The name to show for the connection: the WiFi SSID of the active
 * wireless connection, else the active connection's name. */
static void nm_connection_name(const char *iface, char *name, size_t n)
{
    name[0] = '\0';
    char out[8192];
    char *a1[] = { "nmcli", "-t", "-f", "ACTIVE,SSID", "device", "wifi", "list",
                   "--rescan", "no", NULL };
    if (run(a1, out, sizeof(out), false, 5) == 0) {
        for (char *sp_ = NULL, *line = strtok_r(out, "\n", &sp_); line; line = strtok_r(NULL, "\n", &sp_)) {
            char *f[2];
            if (split_terse(line, f, 2) == 2 && strcmp(f[0], "yes") == 0 && f[1][0]) {
                snprintf(name, n, "%s", f[1]);
                return;
            }
        }
    }
    char *a2[] = { "nmcli", "-t", "-f", "NAME,DEVICE", "connection", "show", "--active", NULL };
    if (run(a2, out, sizeof(out), false, 5) == 0) {
        for (char *sp_ = NULL, *line = strtok_r(out, "\n", &sp_); line; line = strtok_r(NULL, "\n", &sp_)) {
            char *f[2];
            if (split_terse(line, f, 2) != 2 || strcmp(f[1], "lo") == 0) continue;
            if (!name[0] || (iface && strcmp(f[1], iface) == 0)) {
                snprintf(name, n, "%s", f[0]);
                if (iface && strcmp(f[1], iface) == 0) return;
            }
        }
    }
}

/* Work out the state now; returns whether the network is up. */
static bool probe(char *ssid, size_t ssid_sz, char *ip, size_t ip_sz, bool *v6, bool *nm)
{
    char iface[64] = "";
    bool route = default_route_iface(iface, sizeof(iface));
    *nm = nm_running();
    bool up;
    if (*nm) {
        char out[128];
        char *argv[] = { "nmcli", "-t", "-f", "STATE", "general", NULL };
        up = run(argv, out, sizeof(out), false, 5) == 0 && strncmp(out, "connected", 9) == 0;
    } else {
        up = route;
    }
    read_addresses(route ? iface : NULL, ip, ip_sz, v6);
    ssid[0] = '\0';
    if (up) {
        if (*nm) nm_connection_name(route ? iface : NULL, ssid, ssid_sz);
        if (!ssid[0]) snprintf(ssid, ssid_sz, "%s", route ? iface : "network");
    }
    return up;
}

static void set_state(wifi_state_t st)
{
    wifi_state_callback_t cb;
    pthread_mutex_lock(&s_mu);
    s_state = st;
    cb = s_callback;
    pthread_mutex_unlock(&s_mu);
    if (cb) cb(st);
}

/* Refresh the cached state; report a change unless a connect runs. */
static void refresh(bool report)
{
    char ssid[sizeof(s_ssid)], ip[sizeof(s_ip)];
    bool v6, nm;
    bool up = probe(ssid, sizeof(ssid), ip, sizeof(ip), &v6, &nm);
    pthread_mutex_lock(&s_mu);
    bool was_up = s_state == WIFI_STATE_CONNECTED;
    bool busy = s_busy;
    s_nm = nm;
    if (!busy) {
        snprintf(s_ssid, sizeof(s_ssid), "%s", ssid);
        snprintf(s_ip, sizeof(s_ip), "%s", ip);
        s_ipv6 = v6;
    }
    pthread_mutex_unlock(&s_mu);
    if (busy) return;
    if (up && !was_up) {
        if (report) set_state(WIFI_STATE_CONNECTED);
        else s_state = WIFI_STATE_CONNECTED;
    } else if (!up && was_up) {
        if (report) set_state(WIFI_STATE_DISCONNECTED);
        else s_state = WIFI_STATE_DISCONNECTED;
    }
}

static void *monitor_main(void *arg)
{
    (void)arg;
    for (;;) {
        struct timespec ts = { MONITOR_MS / 1000, (MONITOR_MS % 1000) * 1000000L };
        nanosleep(&ts, NULL);
        refresh(true);
    }
    return NULL;
}

/* ---- credentials, as on the device ---------------------------------------- */

/* <data folder>/wifi.cfg; the data folder is the current directory. */
static void wifi_cfg_path(char *out, size_t n)
{
    const char *mp = sd_card_get_mount_point();
    snprintf(out, n, "%s/" WIFI_CFG_NAME, mp && mp[0] ? mp : ".");
}

static bool load_from_file(char *ssid, size_t ssid_sz, char *pass, size_t pass_sz)
{
    char path[64];
    wifi_cfg_path(path, sizeof(path));
    char *buf = NULL;
    size_t len = 0;
    if (sd_card_read_file(path, &buf, &len) != ESP_OK || !buf) return false;
    char *nl = strchr(buf, '\n');
    if (!nl) { free(buf); return false; }
    *nl = '\0';
    char *cr = strchr(buf, '\r');
    if (cr) *cr = '\0';
    snprintf(ssid, ssid_sz, "%s", buf);
    char *p2 = nl + 1;
    p2[strcspn(p2, "\r\n")] = '\0';
    snprintf(pass, pass_sz, "%s", p2);
    free(buf);
    return ssid[0] != '\0';
}

static bool load_from_nvs(char *ssid, size_t ssid_sz, char *pass, size_t pass_sz)
{
    nvs_handle_t h;
    if (nvs_open("wifi", NVS_READONLY, &h) != ESP_OK) return false;
    bool ok = nvs_get_str(h, "ssid", ssid, &ssid_sz) == ESP_OK &&
              nvs_get_str(h, "pass", pass, &pass_sz) == ESP_OK;
    nvs_close(h);
    return ok && ssid[0];
}

static void save_to_nvs(const char *ssid, const char *pass)
{
    nvs_handle_t h;
    if (nvs_open("wifi", NVS_READWRITE, &h) != ESP_OK) return;
    nvs_set_str(h, "ssid", ssid);
    nvs_set_str(h, "pass", pass);
    nvs_commit(h);
    nvs_close(h);
}

static bool load_configured(char *ssid, size_t ssid_sz, char *pass, size_t pass_sz)
{
    if (load_from_file(ssid, ssid_sz, pass, pass_sz)) return true;
    return load_from_nvs(ssid, ssid_sz, pass, pass_sz);
}

/* ---- API ------------------------------------------------------------------ */

esp_err_t wifi_manager_init(void)
{
    if (s_started) return ESP_OK;
    s_started = true;
    refresh(false);
    ESP_LOGI(TAG, "Network %s%s", s_state == WIFI_STATE_CONNECTED ? "up" : "down",
             s_nm ? " (NetworkManager)" : " (no NetworkManager)");
    pthread_t th;
    pthread_attr_t a;
    pthread_attr_init(&a);
    pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED);
    if (pthread_create(&th, &a, monitor_main, NULL) != 0)
        ESP_LOGW(TAG, "No network monitor thread");
    pthread_attr_destroy(&a);
    return ESP_OK;
}

esp_err_t wifi_manager_connect(void)
{
    char ssid[33] = "", pass[65] = "";
    if (!load_configured(ssid, sizeof(ssid), pass, sizeof(pass))) {
        ESP_LOGW(TAG, "No WiFi credentials (wifi.cfg or settings)");
        return ESP_ERR_NOT_FOUND;
    }
    return wifi_manager_connect_to(ssid, pass, false);
}

/* Blocking, like the device's (the editor calls it from a worker). */
esp_err_t wifi_manager_connect_to(const char *ssid, const char *password, bool save)
{
    if (!ssid || !ssid[0]) return ESP_ERR_INVALID_ARG;
    if (pthread_mutex_trylock(&s_op_mu) != 0) return ESP_ERR_INVALID_STATE;
    pthread_mutex_lock(&s_mu);
    bool nm = s_nm;
    s_auth_failed = false;
    s_busy = true;
    pthread_mutex_unlock(&s_mu);

    if (!nm) nm = nm_running();
    if (!nm) {
        ESP_LOGW(TAG, "NetworkManager is not running: WiFi is managed by the system");
        pthread_mutex_lock(&s_mu);
        s_busy = false;
        pthread_mutex_unlock(&s_mu);
        set_state(WIFI_STATE_ERROR);
        refresh(true);
        pthread_mutex_unlock(&s_op_mu);
        return ESP_ERR_NOT_SUPPORTED;
    }

    set_state(WIFI_STATE_CONNECTING);
    char wait_s[8];
    snprintf(wait_s, sizeof(wait_s), "%d", CONNECT_TIMEOUT_S);
    /* The password is an argument of nmcli: other users of this
     * computer could see it in the process list while nmcli runs. */
    char *argv[12];
    int n = 0;
    argv[n++] = "nmcli";
    argv[n++] = "--wait";
    argv[n++] = wait_s;
    argv[n++] = "device";
    argv[n++] = "wifi";
    argv[n++] = "connect";
    argv[n++] = (char *)ssid;
    if (password && password[0]) {
        argv[n++] = "password";
        argv[n++] = (char *)password;
    }
    argv[n] = NULL;
    char out[2048];
    int rc = run(argv, out, sizeof(out), true, CONNECT_TIMEOUT_S + 5);
    bool ok = rc == 0;
    if (!ok) {
        ESP_LOGW(TAG, "nmcli: %s", out[0] ? out : "failed");
        bool auth = strstr(out, "Secrets were required") || strstr(out, "secrets") ||
                    strstr(out, "password") || strstr(out, "802-11-wireless-security");
        pthread_mutex_lock(&s_mu);
        s_auth_failed = auth;
        pthread_mutex_unlock(&s_mu);
    }
    if (ok && save) save_to_nvs(ssid, password ? password : "");

    char nssid[sizeof(s_ssid)], ip[sizeof(s_ip)];
    bool v6, nmr;
    bool up = probe(nssid, sizeof(nssid), ip, sizeof(ip), &v6, &nmr);
    pthread_mutex_lock(&s_mu);
    snprintf(s_ssid, sizeof(s_ssid), "%s", ok ? ssid : nssid);
    snprintf(s_ip, sizeof(s_ip), "%s", ip);
    s_ipv6 = v6;
    s_busy = false;
    pthread_mutex_unlock(&s_mu);
    set_state(ok && up ? WIFI_STATE_CONNECTED : WIFI_STATE_ERROR);
    pthread_mutex_unlock(&s_op_mu);
    return ok ? ESP_OK : ESP_FAIL;
}

esp_err_t wifi_manager_disconnect(void)
{
    pthread_mutex_lock(&s_mu);
    bool nm = s_nm;
    pthread_mutex_unlock(&s_mu);
    if (!nm) {
        ESP_LOGW(TAG, "NetworkManager is not running: cannot disconnect");
        return ESP_ERR_NOT_SUPPORTED;
    }
    char out[4096];
    char *list[] = { "nmcli", "-t", "-f", "DEVICE,TYPE,STATE", "device", NULL };
    if (run(list, out, sizeof(out), false, 5) != 0) return ESP_FAIL;
    bool any = false;
    for (char *sp_ = NULL, *line = strtok_r(out, "\n", &sp_); line; line = strtok_r(NULL, "\n", &sp_)) {
        char *f[3];
        if (split_terse(line, f, 3) != 3) continue;
        if (strcmp(f[1], "wifi") != 0 || strncmp(f[2], "connected", 9) != 0) continue;
        char *dc[] = { "nmcli", "device", "disconnect", f[0], NULL };
        char o2[512];
        if (run(dc, o2, sizeof(o2), true, 15) == 0) any = true;
    }
    refresh(true);
    return any ? ESP_OK : ESP_FAIL;
}

wifi_state_t wifi_manager_get_state(void)
{
    pthread_mutex_lock(&s_mu);
    wifi_state_t st = s_state;
    pthread_mutex_unlock(&s_mu);
    return st;
}

bool wifi_manager_is_connected(void)
{
    return wifi_manager_get_state() == WIFI_STATE_CONNECTED;
}

void wifi_manager_set_callback(wifi_state_callback_t cb)
{
    pthread_mutex_lock(&s_mu);
    s_callback = cb;
    pthread_mutex_unlock(&s_mu);
}

/* The strings stay valid: the buffers are static; a refresh may
 * change them under a reader, which at worst shows a mix for one
 * frame (as with the device's event task). */
const char *wifi_manager_get_ip(void) { return s_ip; }
const char *wifi_manager_get_ssid(void) { return s_ssid; }
bool wifi_manager_last_failure_was_auth(void) { return s_auth_failed; }
bool wifi_manager_has_global_ipv6(void) { return s_ipv6; }

static int cmp_rssi(const void *a, const void *b)
{
    return ((const wifi_scan_result_t *)b)->rssi - ((const wifi_scan_result_t *)a)->rssi;
}

esp_err_t wifi_manager_scan(wifi_scan_result_t *results, int max_results, int *out_count)
{
    *out_count = 0;
    if (pthread_mutex_trylock(&s_op_mu) != 0) return ESP_ERR_INVALID_STATE;
    if (!nm_running()) {
        pthread_mutex_unlock(&s_op_mu);
        ESP_LOGW(TAG, "NetworkManager is not running: no WiFi scan");
        return ESP_ERR_NOT_SUPPORTED;
    }
    static char out[65536];
    char *argv[] = { "nmcli", "-t", "-f", "SSID,SIGNAL,SECURITY", "device", "wifi",
                     "list", "--rescan", "yes", NULL };
    int rc = run(argv, out, sizeof(out), false, 30);
    if (rc != 0) {
        pthread_mutex_unlock(&s_op_mu);
        return ESP_FAIL;
    }
    int n = 0;
    for (char *sp_ = NULL, *line = strtok_r(out, "\n", &sp_); line; line = strtok_r(NULL, "\n", &sp_)) {
        char *f[3];
        if (split_terse(line, f, 3) != 3 || !f[0][0]) continue;
        int signal = atoi(f[1]);
        int rssi = signal / 2 - 100;   /* NetworkManager's mapping, reversed */
        bool open = !f[2][0] || strcmp(f[2], "--") == 0;
        int dup = -1;
        for (int i = 0; i < n; i++)
            if (strcmp(results[i].ssid, f[0]) == 0) { dup = i; break; }
        if (dup >= 0) {
            if (rssi > results[dup].rssi) results[dup].rssi = (int8_t)rssi;
            continue;
        }
        if (n >= max_results) continue;
        snprintf(results[n].ssid, sizeof(results[n].ssid), "%s", f[0]);
        results[n].rssi = (int8_t)rssi;
        results[n].open = open;
        n++;
    }
    qsort(results, (size_t)n, sizeof(results[0]), cmp_rssi);
    *out_count = n;
    pthread_mutex_unlock(&s_op_mu);
    return ESP_OK;
}

esp_err_t wifi_manager_save_to_file(const char *ssid, const char *password)
{
    if (!ssid) return ESP_ERR_INVALID_ARG;
    char buf[33 + 1 + 64 + 1 + 1];
    int len = snprintf(buf, sizeof(buf), "%s\n%s\n", ssid, password ? password : "");
    if (len < 0 || (size_t)len >= sizeof(buf)) return ESP_ERR_INVALID_SIZE;
    char path[64];
    wifi_cfg_path(path, sizeof(path));
    return sd_card_write_file(path, buf, (size_t)len);
}

bool wifi_manager_get_configured_ssid(char *ssid, size_t ssid_sz)
{
    char p[65];
    if (!ssid || ssid_sz == 0) return false;
    bool ok = load_configured(ssid, ssid_sz, p, sizeof(p));
    if (!ok) ssid[0] = '\0';
    return ok;
}

bool wifi_manager_get_configured(char *ssid, size_t ssid_sz, char *pass, size_t pass_sz)
{
    bool ok = load_configured(ssid, ssid_sz, pass, pass_sz);
    if (!ok) {
        if (ssid_sz) ssid[0] = '\0';
        if (pass_sz) pass[0] = '\0';
    }
    return ok;
}

/* Erases Draftling's saved credentials; NetworkManager's own saved
 * connections are the system's and stay. */
esp_err_t wifi_manager_forget(void)
{
    nvs_handle_t h;
    if (nvs_open("wifi", NVS_READWRITE, &h) == ESP_OK) {
        nvs_erase_all(h);
        nvs_commit(h);
        nvs_close(h);
    }
    return ESP_OK;
}
