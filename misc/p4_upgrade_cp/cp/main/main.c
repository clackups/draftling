/*
 * ESP-Hosted co-processor (Wi-Fi + BLE controller) for an ESP32-P4 host.
 *
 * The esp_hosted component starts the SDIO transport, the RPC server,
 * Wi-Fi and the BLE controller on its own; the application only has to
 * bring up NVS (Wi-Fi keeps its calibration and settings there).
 */
#include "esp_app_desc.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "cp_hosted";

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP-Hosted %s co-processor (Wi-Fi + BLE) running",
             esp_app_get_description()->version);
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
