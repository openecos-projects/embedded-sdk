/* USBFS controller lifecycle/status backend.  This layer intentionally does
 * not claim a USB device class or descriptor stack; applications must add a
 * class implementation above this controller contract. */
#include "ecos/hal/usb.h"

#include <stddef.h>
#include <stdint.h>

#define REG32(address) (*(volatile uint32_t *)(uintptr_t)(address))
#define RCC_CTLR      REG32(0x40021000u)
#define RCC_AHBPCENR  REG32(0x40021014u)
#define EXTEN_CTR     REG32(0x40023800u)
#define RCC_CFGR0     REG32(0x40021004u)
#define USB_BASE      0x40023400u
#define USB_CTRL      (*(volatile uint8_t *)(uintptr_t)(USB_BASE + 0x00u))
#define USB_DEV_CTRL  (*(volatile uint8_t *)(uintptr_t)(USB_BASE + 0x01u))
#define USB_INT_EN    (*(volatile uint8_t *)(uintptr_t)(USB_BASE + 0x02u))
#define USB_INT_FG    (*(volatile uint8_t *)(uintptr_t)(USB_BASE + 0x06u))
#define USB_INT_ST    (*(volatile uint8_t *)(uintptr_t)(USB_BASE + 0x07u))
#define USB_DEV_AD    (*(volatile uint8_t *)(uintptr_t)(USB_BASE + 0x03u))
#define EXTEN_USBFS_IO_EN (1u << 2)
#define RCC_AHB_USBFS     (1u << 12)
#define RCC_HSION         (1u << 0)
#define RCC_HSIRDY        (1u << 1)
#define RCC_PLLON         (1u << 24)
#define RCC_PLLRDY        (1u << 25)
#define RCC_PLLSRC        (1u << 16)
#define RCC_PLLXTPRE      (1u << 17)
#define RCC_PLLMULL       (0x003C0000u)
#define RCC_USBPRE        (1u << 22)
#define EXTEN_PLL_HSI_PRE (1u << 4)
#define USB_CLOCK_TIMEOUT 200000u
#define USB_RESET_SIE     0x04u
#define USB_CLR_ALL       0x02u
#define USB_DEV_PU_EN     0x20u
#define USB_INT_BUSY      0x08u
#define USB_DMA_EN        0x01u
#define USB_PD_DIS        0x80u
#define USB_PORT_EN       0x01u

static uint8_t initialized;
static int valid_id(hal_usb_id_t id) { return id == HAL_USB_ID_0; }

static ecos_err_t enable_usb_clock(void)
{
    uint32_t timeout = USB_CLOCK_TIMEOUT;

    RCC_CTLR |= RCC_HSION;
    while ((RCC_CTLR & RCC_HSIRDY) == 0u) {
        if (timeout-- == 0u) return ECOS_ERR_TIMEOUT;
    }
    /* Keep SYSCLK on HSI.  USB uses PLLCLK independently; HSI/2 x 12 is
     * the documented 48 MHz source and preserves the console's 8 MHz BRR.
     * WCH's RCC_USBCLKConfig(PLLCLK_Div1) sets USBPRE. */
    if ((RCC_CFGR0 & 0x0Cu) == 0u) {
        EXTEN_CTR &= ~EXTEN_PLL_HSI_PRE; /* PLL input = HSI/2 */
        RCC_CFGR0 = (RCC_CFGR0 & ~(RCC_PLLSRC | RCC_PLLXTPRE | RCC_PLLMULL)) |
                     (10u << 18);
        RCC_CFGR0 |= RCC_USBPRE; /* PLLCLK / 1 = 48 MHz */

        RCC_CTLR |= RCC_PLLON;
        while ((RCC_CTLR & RCC_PLLRDY) == 0u) {
            if (timeout-- == 0u) return ECOS_ERR_TIMEOUT;
        }
    } else if ((RCC_CFGR0 & RCC_PLLMULL) != (10u << 18) ||
               (RCC_CFGR0 & RCC_USBPRE) == 0u) {
        return ECOS_ERR_INVALID_STATE;
    }
    return ECOS_OK;
}

ecos_err_t hal_usb_init(hal_usb_id_t id, const hal_usb_config_t *config)
{
    if (!valid_id(id) || config == NULL || config->low_speed > 1u ||
        config->internal_pullup > 1u)
        return ECOS_ERR_INVALID_ARGUMENT;
    {
        ecos_err_t result = enable_usb_clock();
        if (result != ECOS_OK)
            return result;
    }
    RCC_AHBPCENR |= RCC_AHB_USBFS;
    EXTEN_CTR |= EXTEN_USBFS_IO_EN;
    USB_CTRL = USB_RESET_SIE | USB_CLR_ALL;
    USB_CTRL = 0u;
    /* No USB class/endpoint dispatcher is registered by this controller
     * layer; keep interrupt sources masked to avoid an unhandled IRQ storm. */
    USB_INT_EN = 0u;
    USB_CTRL = USB_INT_BUSY | USB_DMA_EN |
               (config->internal_pullup != 0u ? USB_DEV_PU_EN : 0u);
    USB_DEV_CTRL = USB_PD_DIS | USB_PORT_EN |
                   (config->low_speed != 0u ? 0x04u : 0u);
    USB_DEV_AD = 0u;
    initialized = 1u;
    return ECOS_OK;
}

ecos_err_t hal_usb_deinit(hal_usb_id_t id)
{
    if (!valid_id(id)) return ECOS_ERR_INVALID_ARGUMENT;
    USB_CTRL = USB_RESET_SIE | USB_CLR_ALL;
    USB_INT_EN = 0u;
    initialized = 0u;
    return ECOS_OK;
}

ecos_err_t hal_usb_get_status(hal_usb_id_t id, uint32_t *status)
{
    if (!valid_id(id) || status == NULL) return ECOS_ERR_INVALID_ARGUMENT;
    if (initialized == 0u) return ECOS_ERR_NOT_INITIALIZED;
    *status = (uint32_t)USB_INT_ST | ((uint32_t)USB_INT_FG << 8) |
              ((uint32_t)USB_DEV_AD << 16);
    return ECOS_OK;
}

ecos_err_t hal_usb_clear_interrupts(hal_usb_id_t id, uint8_t flags)
{
    if (!valid_id(id)) return ECOS_ERR_INVALID_ARGUMENT;
    if (initialized == 0u) return ECOS_ERR_NOT_INITIALIZED;
    USB_INT_FG = flags;
    return ECOS_OK;
}
