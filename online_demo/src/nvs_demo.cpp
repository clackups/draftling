/*
 * Online demo: NVS (the device's settings store) in the browser.
 *
 * Every namespace / key / typed value is kept in memory and written to
 * DEMO_NVS_FILE, which lives on an IndexedDB-backed file system
 * mounted by web/demo.js; each simulated board has its own (the SD card
 * is shared, like a card moved between devices), so settings (font size, theme, active
 * layouts, ...) survive a page reload the way they survive a reboot.
 * nvs_flash_erase() (Factory reset) deletes them all.
 */

#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include "demo_host.h"
#include "nvs.h"
#include "nvs_flash.h"

#ifndef DEMO_BOARD
#define DEMO_BOARD "board"
#endif
#define DEMO_NVS_FILE "/persist/nvs-" DEMO_BOARD ".bin"

namespace {

enum vtype : uint8_t { T_U8 = 1, T_I8, T_U16, T_I16, T_U32, T_I32, T_U64, T_STR, T_BLOB };

struct value {
    uint8_t type;
    std::vector<uint8_t> data;
};

/* key: "<namespace>\n<key>" */
std::map<std::string, value> g_store;
std::vector<std::string> g_handles;   /* index + 1 = handle */

std::string full_key(nvs_handle_t h, const char *key)
{
    return g_handles[h - 1] + "\n" + key;
}

bool valid(nvs_handle_t h)
{
    return h >= 1 && h <= g_handles.size();
}

void save()
{
    FILE *f = fopen(DEMO_NVS_FILE, "wb");
    if (!f) return;
    for (const auto &kv : g_store) {
        uint32_t klen = (uint32_t)kv.first.size();
        uint32_t vlen = (uint32_t)kv.second.data.size();
        fwrite(&klen, 4, 1, f);
        fwrite(kv.first.data(), 1, klen, f);
        fwrite(&kv.second.type, 1, 1, f);
        fwrite(&vlen, 4, 1, f);
        if (vlen) fwrite(kv.second.data.data(), 1, vlen, f);
    }
    fclose(f);
    demo_host_storage_dirty();
}

void load()
{
    g_store.clear();
    FILE *f = fopen(DEMO_NVS_FILE, "rb");
    if (!f) return;
    for (;;) {
        uint32_t klen, vlen;
        uint8_t type;
        if (fread(&klen, 4, 1, f) != 1 || klen > 4096) break;
        std::string k(klen, '\0');
        if (fread(&k[0], 1, klen, f) != klen) break;
        if (fread(&type, 1, 1, f) != 1) break;
        if (fread(&vlen, 4, 1, f) != 1 || vlen > (1u << 20)) break;
        value v;
        v.type = type;
        v.data.resize(vlen);
        if (vlen && fread(v.data.data(), 1, vlen, f) != vlen) break;
        g_store[k] = std::move(v);
    }
    fclose(f);
}

esp_err_t set_raw(nvs_handle_t h, const char *key, uint8_t type,
                  const void *p, size_t n)
{
    if (!valid(h) || !key) return ESP_ERR_INVALID_ARG;
    value v;
    v.type = type;
    v.data.assign((const uint8_t *)p, (const uint8_t *)p + n);
    g_store[full_key(h, key)] = std::move(v);
    return ESP_OK;
}

esp_err_t get_raw(nvs_handle_t h, const char *key, uint8_t type,
                  void *p, size_t n)
{
    if (!valid(h) || !key) return ESP_ERR_INVALID_ARG;
    auto it = g_store.find(full_key(h, key));
    if (it == g_store.end() || it->second.type != type ||
        it->second.data.size() != n) {
        return ESP_ERR_NVS_NOT_FOUND;
    }
    memcpy(p, it->second.data.data(), n);
    return ESP_OK;
}

esp_err_t get_var(nvs_handle_t h, const char *key, uint8_t type,
                  void *out, size_t *len)
{
    if (!valid(h) || !key || !len) return ESP_ERR_INVALID_ARG;
    auto it = g_store.find(full_key(h, key));
    if (it == g_store.end() || it->second.type != type) return ESP_ERR_NVS_NOT_FOUND;
    size_t n = it->second.data.size();
    if (!out) {
        *len = n;
        return ESP_OK;
    }
    if (*len < n) return ESP_ERR_INVALID_SIZE;
    memcpy(out, it->second.data.data(), n);
    *len = n;
    return ESP_OK;
}

}  // namespace

extern "C" {

esp_err_t nvs_flash_init(void)
{
    load();
    return ESP_OK;
}

esp_err_t nvs_flash_erase(void)
{
    g_store.clear();
    remove(DEMO_NVS_FILE);
    demo_host_storage_dirty();
    return ESP_OK;
}

esp_err_t nvs_open(const char *ns, nvs_open_mode_t mode, nvs_handle_t *out)
{
    (void)mode;
    if (!ns || !out) return ESP_ERR_INVALID_ARG;
    for (size_t i = 0; i < g_handles.size(); i++) {
        if (g_handles[i] == ns) {
            *out = (nvs_handle_t)(i + 1);
            return ESP_OK;
        }
    }
    g_handles.push_back(ns);
    *out = (nvs_handle_t)g_handles.size();
    return ESP_OK;
}

void nvs_close(nvs_handle_t h)
{
    (void)h;
}

esp_err_t nvs_commit(nvs_handle_t h)
{
    if (!valid(h)) return ESP_ERR_INVALID_ARG;
    save();
    return ESP_OK;
}

esp_err_t nvs_erase_key(nvs_handle_t h, const char *key)
{
    if (!valid(h) || !key) return ESP_ERR_INVALID_ARG;
    return g_store.erase(full_key(h, key)) ? ESP_OK : ESP_ERR_NVS_NOT_FOUND;
}

esp_err_t nvs_erase_all(nvs_handle_t h)
{
    if (!valid(h)) return ESP_ERR_INVALID_ARG;
    std::string prefix = g_handles[h - 1] + "\n";
    for (auto it = g_store.begin(); it != g_store.end();) {
        if (it->first.compare(0, prefix.size(), prefix) == 0) it = g_store.erase(it);
        else ++it;
    }
    return ESP_OK;
}

#define NVS_INT(suffix, ctype, tag)                                              \
    esp_err_t nvs_get_##suffix(nvs_handle_t h, const char *key, ctype *v)        \
    {                                                                            \
        return get_raw(h, key, tag, v, sizeof(*v));                              \
    }                                                                            \
    esp_err_t nvs_set_##suffix(nvs_handle_t h, const char *key, ctype v)         \
    {                                                                            \
        return set_raw(h, key, tag, &v, sizeof(v));                              \
    }

NVS_INT(u8, uint8_t, T_U8)
NVS_INT(i8, int8_t, T_I8)
NVS_INT(u16, uint16_t, T_U16)
NVS_INT(i16, int16_t, T_I16)
NVS_INT(u32, uint32_t, T_U32)
NVS_INT(i32, int32_t, T_I32)
NVS_INT(u64, uint64_t, T_U64)

esp_err_t nvs_get_str(nvs_handle_t h, const char *key, char *out, size_t *len)
{
    return get_var(h, key, T_STR, out, len);
}

esp_err_t nvs_set_str(nvs_handle_t h, const char *key, const char *v)
{
    if (!v) return ESP_ERR_INVALID_ARG;
    return set_raw(h, key, T_STR, v, strlen(v) + 1);
}

esp_err_t nvs_get_blob(nvs_handle_t h, const char *key, void *out, size_t *len)
{
    return get_var(h, key, T_BLOB, out, len);
}

esp_err_t nvs_set_blob(nvs_handle_t h, const char *key, const void *v, size_t len)
{
    return set_raw(h, key, T_BLOB, v, len);
}

}  // extern "C"
