#pragma once
#include "esp_err.h"
/* Linux: TLS uses the operating system's CA certificates
 * (src/http_client_linux.c); nothing to attach. */
static inline esp_err_t esp_crt_bundle_attach(void *conf) { (void)conf; return ESP_OK; }
