/* Linux: the part of ESP-IDF's esp_http_client the Git client and
 * Draftbox use, over the host's TCP/IP stack and OpenSSL
 * (src/http_client_linux.c). Same streaming model: open() sends the
 * request line and headers (the body length is known up front),
 * write() sends the body, fetch_headers() reads the response head,
 * read() the body. */
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
    HTTP_METHOD_GET = 0,
    HTTP_METHOD_POST,
} esp_http_client_method_t;
typedef enum {
    HTTP_ADDR_TYPE_UNSPEC = 0,
    HTTP_ADDR_TYPE_INET,
    HTTP_ADDR_TYPE_INET6,
} esp_http_client_addr_type_t;
typedef struct {
    const char *url;
    esp_http_client_method_t method;
    esp_err_t (*crt_bundle_attach)(void *conf);
    int timeout_ms;
    int buffer_size;
    int buffer_size_tx;
    bool keep_alive_enable;
    esp_http_client_addr_type_t addr_type;
} esp_http_client_config_t;
typedef struct linux_http_client *esp_http_client_handle_t;
esp_http_client_handle_t esp_http_client_init(const esp_http_client_config_t *cfg);
esp_err_t esp_http_client_set_header(esp_http_client_handle_t c, const char *key, const char *value);
esp_err_t esp_http_client_get_header(esp_http_client_handle_t c, const char *key, char **value);
esp_err_t esp_http_client_open(esp_http_client_handle_t c, int write_len);
int esp_http_client_write(esp_http_client_handle_t c, const char *buf, int len);
int64_t esp_http_client_fetch_headers(esp_http_client_handle_t c);
int esp_http_client_get_status_code(esp_http_client_handle_t c);
int esp_http_client_read(esp_http_client_handle_t c, char *buf, int len);
esp_err_t esp_http_client_close(esp_http_client_handle_t c);
esp_err_t esp_http_client_cleanup(esp_http_client_handle_t c);
#ifdef __cplusplus
}
#endif
