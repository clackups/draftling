// Host stand-ins: SD card on the local filesystem, FreeRTOS task, and an
// esp_http_client that runs `git http-backend` as a CGI.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <dirent.h>
#include <sys/stat.h>
#include <strings.h>
#include <unistd.h>
#include "sd_card.h"
#include "esp_http_client.h"
#include "freertos/task.h"

extern "C" bool sd_card_is_ready(void) { return true; }
extern "C" esp_err_t sd_card_read_file(const char *path, char **out, size_t *len)
{
    FILE *f = fopen(path, "rb"); if (!f) return ESP_ERR_NOT_FOUND;
    std::string s; char b[4096]; size_t r;
    while ((r = fread(b, 1, sizeof b, f)) > 0) s.append(b, r);
    fclose(f);
    *out = (char *)malloc(s.size() + 1); memcpy(*out, s.data(), s.size()); (*out)[s.size()] = 0;
    if (len) *len = s.size();
    return ESP_OK;
}
extern "C" esp_err_t sd_card_write_file(const char *path, const char *d, size_t n)
{
    FILE *f = fopen(path, "wb"); if (!f) return ESP_FAIL;
    bool ok = fwrite(d, 1, n, f) == n; if (fclose(f)) ok = false;
    return ok ? ESP_OK : ESP_FAIL;
}
extern "C" bool sd_card_file_exists(const char *p) { struct stat st; return stat(p, &st) == 0; }
extern "C" esp_err_t sd_card_delete_file(const char *p) { return remove(p) == 0 ? ESP_OK : ESP_FAIL; }
extern "C" esp_err_t sd_card_list_dir(const char *path, sd_card_file_entry_t **out, int *count)
{
    *out = NULL; *count = 0;
    DIR *d = opendir(path); if (!d) return ESP_FAIL;
    std::vector<sd_card_file_entry_t> v; struct dirent *e;
    while ((e = readdir(d))) {
        if (e->d_name[0] == '.') continue;
        sd_card_file_entry_t x = {}; snprintf(x.name, sizeof x.name, "%s", e->d_name);
        std::string full = std::string(path) + "/" + e->d_name; struct stat st;
        if (stat(full.c_str(), &st) == 0) { x.is_dir = S_ISDIR(st.st_mode); x.size = st.st_size; }
        v.push_back(x);
    }
    closedir(d);
    std::sort(v.begin(), v.end(), [](auto &a, auto &b) {
        if (a.is_dir != b.is_dir) return a.is_dir; return strcasecmp(a.name, b.name) < 0; });
    if (!v.empty()) {
        *out = (sd_card_file_entry_t *)malloc(v.size() * sizeof(v[0]));
        memcpy(*out, v.data(), v.size() * sizeof(v[0]));
    }
    *count = (int)v.size();
    return ESP_OK;
}
extern "C" void sd_card_free_list(sd_card_file_entry_t *l) { free(l); }

/* ---- tasks: run synchronously ---- */
extern "C" void vTaskDelete(TaskHandle_t t) { (void)t; }
extern "C" TaskHandle_t xTaskCreateStaticPinnedToCore(void (*fn)(void *), const char *, uint32_t,
                                                      void *arg, int, StackType_t *, StaticTask_t *, int)
{
    fn(arg);
    return (TaskHandle_t)1;
}

/* ---- fake HTTP over git http-backend ---- */
const char *g_http_root = nullptr;   /* GIT_PROJECT_ROOT */
int g_http_requests = 0;
struct fake_http {
    std::string url, body, resp, ctype, gitproto;
    std::map<std::string, std::string> hdr;
    int method, status = 0; size_t pos = 0; bool done = false;
};
extern "C" esp_http_client_handle_t esp_http_client_init(const esp_http_client_config_t *c)
{ auto *h = new fake_http; h->url = c->url; h->method = c->method; return h; }
extern "C" esp_err_t esp_http_client_set_header(esp_http_client_handle_t c, const char *k, const char *v)
{ c->hdr[k] = v; return ESP_OK; }
extern "C" esp_err_t esp_http_client_get_header(esp_http_client_handle_t c, const char *k, char **v)
{ *v = nullptr; if (!strcasecmp(k, "Content-Type")) *v = (char *)c->ctype.c_str(); return ESP_OK; }
extern "C" esp_err_t esp_http_client_open(esp_http_client_handle_t c, int) { (void)c; return ESP_OK; }
extern "C" int esp_http_client_write(esp_http_client_handle_t c, const char *b, int n) { c->body.append(b, n); return n; }
extern "C" int64_t esp_http_client_fetch_headers(esp_http_client_handle_t c)
{
    if (c->done) return 0;
    c->done = true; g_http_requests++;
    std::string u = c->url; size_t p = u.find("://"); p = u.find('/', p + 3);
    std::string path = u.substr(p), query;
    size_t q = path.find('?'); if (q != std::string::npos) { query = path.substr(q + 1); path = path.substr(0, q); }
    std::string in = std::string(g_http_root) + "/.req", outp = std::string(g_http_root) + "/.resp";
    FILE *f = fopen(in.c_str(), "wb"); fwrite(c->body.data(), 1, c->body.size(), f); fclose(f);
    std::string gp = c->hdr.count("Git-Protocol") ? c->hdr["Git-Protocol"] : "";
    std::string ct = c->hdr.count("Content-Type") ? c->hdr["Content-Type"] : "";
    char cmd[4096];
    snprintf(cmd, sizeof cmd, "env GIT_PROJECT_ROOT='%s' GIT_HTTP_EXPORT_ALL=1 REMOTE_USER=t "
             "REQUEST_METHOD=%s PATH_INFO='%s' QUERY_STRING='%s' CONTENT_TYPE='%s' "
             "CONTENT_LENGTH=%zu GIT_PROTOCOL='%s' git http-backend < '%s' > '%s' 2>/dev/null",
             g_http_root, c->method == HTTP_METHOD_POST ? "POST" : "GET", path.c_str(), query.c_str(),
             ct.c_str(), c->body.size(), gp.c_str(), in.c_str(), outp.c_str());
    if (system(cmd) != 0) { c->status = 500; return 0; }
    char *data; size_t len;
    sd_card_read_file(outp.c_str(), &data, &len);
    std::string all(data, len); free(data);
    size_t he = all.find("\r\n\r\n"); size_t skip = 4;
    if (he == std::string::npos) { he = all.find("\n\n"); skip = 2; }
    std::string head = all.substr(0, he); c->resp = all.substr(he + skip);
    c->status = 200;
    size_t sp = head.find("Status: "); if (sp != std::string::npos) c->status = atoi(head.c_str() + sp + 8);
    size_t cp = head.find("Content-Type: ");
    if (cp != std::string::npos) c->ctype = head.substr(cp + 14, head.find_first_of("\r\n", cp) - cp - 14);
    return (int64_t)c->resp.size();
}
extern "C" int esp_http_client_get_status_code(esp_http_client_handle_t c) { return c->status; }
extern "C" int esp_http_client_read(esp_http_client_handle_t c, char *b, int n)
{
    if (!c->done) esp_http_client_fetch_headers(c);
    size_t k = std::min((size_t)n, c->resp.size() - c->pos);
    memcpy(b, c->resp.data() + c->pos, k); c->pos += k; return (int)k;
}
extern "C" esp_err_t esp_http_client_close(esp_http_client_handle_t c) { (void)c; return ESP_OK; }
extern "C" esp_err_t esp_http_client_cleanup(esp_http_client_handle_t c) { delete c; return ESP_OK; }
extern "C" esp_err_t sd_card_replace_with(const char *tmp, const char *path)
{
    remove(path);
    return rename(tmp, path) == 0 ? ESP_OK : ESP_FAIL;
}
