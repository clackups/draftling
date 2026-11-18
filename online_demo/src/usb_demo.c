/*
 * Online demo: the USB device functions of the FNK0104S's native USB
 * port. Both start as on the device, but no computer is ever plugged
 * in: "SD card via USB" switches the card over (the editor then keeps
 * its hands off it until the mode is switched off, which restarts the
 * device), and "Sync via browser" waits for a USB cable.
 */

#include "kermit.h"
#include "usb_msc.h"

static usb_msc_mode_t s_msc_mode = USB_MSC_MODE_OFF;
static bool s_kermit_running;

esp_err_t usb_msc_set_mode(usb_msc_mode_t mode)
{
    s_msc_mode = mode;
    return ESP_OK;
}

usb_msc_mode_t usb_msc_get_mode(void)
{
    return s_msc_mode;
}

esp_err_t kermit_server_start(const char *root_dir, kermit_status_cb_t cb)
{
    (void)root_dir; (void)cb;
    s_kermit_running = true;
    return ESP_OK;
}

void kermit_server_stop(void)
{
    s_kermit_running = false;
}

bool kermit_server_is_running(void)
{
    return s_kermit_running;
}

int kermit_server_usb_connected(void)
{
    return 0;
}
