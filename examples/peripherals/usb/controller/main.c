#include "ecos/bsp/console.h"
#include "ecos/driver/usb.h"
#include "ecos/log.h"

#define LOG_TAG "usb-controller"

int main(void)
{
    const ecos_usb_config_t config = ECOS_USB_CONFIG_DEFAULT;
    uint32_t status;
    ECOS_PANIC_ON_ERROR(LOG_TAG, bsp_console_init(), "initialize console");
    if (ecos_usb_init(ECOS_USB_DEFAULT, &config) == ECOS_ERR_UNSUPPORTED) {
        (void)ECOS_LOGW(LOG_TAG, "USBFS needs a configured 48 MHz PLL clock");
    } else {
        ECOS_PANIC_ON_ERROR(LOG_TAG, ecos_usb_get_status(ECOS_USB_DEFAULT, &status),
                            "read USB status");
        (void)ECOS_LOGI(LOG_TAG, "USB status: 0x%08X", status);
    }
    for (;;) __asm__ volatile("nop");
}
