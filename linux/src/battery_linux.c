/*
 * Linux: the battery is the computer's, from the kernel's power supply
 * class (/sys/class/power_supply/<name>/ of type "Battery"). Without
 * one (a desktop computer) the percentage is -1 and the editor hides
 * the battery indicator.
 */

#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "battery.h"

#define PS_DIR "/sys/class/power_supply"

static char s_bat[256];   /* the battery's folder, "" = none */

static bool read_line(const char *dir, const char *file, char *out, size_t n)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", dir, file);
    FILE *f = fopen(path, "r");
    if (!f) return false;
    bool ok = fgets(out, (int)n, f) != NULL;
    fclose(f);
    if (ok) out[strcspn(out, "\r\n")] = '\0';
    return ok;
}

static void find_battery(void)
{
    s_bat[0] = '\0';
    DIR *d = opendir(PS_DIR);
    if (!d) return;
    struct dirent *e;
    char dir[sizeof(s_bat)], type[32], scope[32];
    while ((e = readdir(d)) != NULL) {
        if (e->d_name[0] == '.') continue;
        snprintf(dir, sizeof(dir), PS_DIR "/%s", e->d_name);
        if (!read_line(dir, "type", type, sizeof(type)) || strcmp(type, "Battery") != 0)
            continue;
        /* Wireless mice and keyboards report "Device" scope. */
        if (read_line(dir, "scope", scope, sizeof(scope)) && strcmp(scope, "Device") == 0)
            continue;
        snprintf(s_bat, sizeof(s_bat), "%s", dir);
        break;
    }
    closedir(d);
}

int battery_init(int gpio_num, int enable_gpio, int divider)
{
    (void)gpio_num; (void)enable_gpio; (void)divider;
    find_battery();
    return 0;
}

int battery_read_mv(void)
{
    char v[32];
    if (!s_bat[0] || !read_line(s_bat, "voltage_now", v, sizeof(v))) return -1;
    return (int)(strtol(v, NULL, 10) / 1000);   /* uV -> mV */
}

int battery_read_percent(void)
{
    char v[32];
    if (!s_bat[0] || !read_line(s_bat, "capacity", v, sizeof(v))) return -1;
    int p = (int)strtol(v, NULL, 10);
    return p < 0 ? 0 : (p > 100 ? 100 : p);
}

int battery_read_charging(void)
{
    char v[32];
    if (!s_bat[0] || !read_line(s_bat, "status", v, sizeof(v))) return -1;
    if (strcmp(v, "Charging") == 0 || strcmp(v, "Full") == 0) return 1;
    if (strcmp(v, "Discharging") == 0 || strcmp(v, "Not charging") == 0) return 0;
    return -1;
}
