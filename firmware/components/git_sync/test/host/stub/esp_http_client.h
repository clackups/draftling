#pragma once
#include "esp_err.h"
#include <stdbool.h>
typedef enum { HTTP_METHOD_GET, HTTP_METHOD_POST } esp_http_client_method_t;
typedef enum { HTTP_ADDR_TYPE_UNSPEC, HTTP_ADDR_TYPE_INET, HTTP_ADDR_TYPE_INET6 } esp_http_client_addr_type_t;
typedef struct {
    const char *url; esp_http_client_method_t method; int (*crt_bundle_attach)(void *);
    int timeout_ms, buffer_size, buffer_size_tx; bool keep_alive_enable;
    esp_http_client_addr_type_t addr_type;
} esp_http_client_config_t;
typedef struct fake_http *esp_http_client_handle_t;
#ifdef __cplusplus
extern "C" {
#endif
esp_http_client_handle_t esp_http_client_init(const esp_http_client_config_t *c);
esp_err_t esp_http_client_set_header(esp_http_client_handle_t c, const char *k, const char *v);
esp_err_t esp_http_client_get_header(esp_http_client_handle_t c, const char *k, char **v);
esp_err_t esp_http_client_open(esp_http_client_handle_t c, int len);
int esp_http_client_write(esp_http_client_handle_t c, const char *b, int n);
int64_t esp_http_client_fetch_headers(esp_http_client_handle_t c);
int esp_http_client_get_status_code(esp_http_client_handle_t c);
int esp_http_client_read(esp_http_client_handle_t c, char *b, int n);
esp_err_t esp_http_client_close(esp_http_client_handle_t c);
esp_err_t esp_http_client_cleanup(esp_http_client_handle_t c);
#ifdef __cplusplus
}
#endif
