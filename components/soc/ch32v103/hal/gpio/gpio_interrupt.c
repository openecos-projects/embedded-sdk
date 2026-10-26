/* CH32V103 EXTI0..15. One GPIO port owns each numbered EXTI line;
 * EXTI5..9 and EXTI10..15 share PFIC sources and must share a priority.
 */
#include "ecos/hal/gpio_interrupt.h"
#include "ecos/hal/interrupt.h"
#include "ch32v103_interrupt_internal.h"

#include <stddef.h>

#define RCC_APB2PCENR 0x40021018u
#define AFIO_EXTICR(index) (0x40010008u + 4u * (index))
#define EXTI_INTENR 0x40010400u
#define EXTI_RTENR 0x40010408u
#define EXTI_FTENR 0x4001040Cu
#define EXTI_INTFR 0x40010414u
#define EXTI_LINE_COUNT 16u
#define EXTI_GROUP_COUNT 7u

typedef struct {
    hal_gpio_interrupt_handler_t handler;
    void *arg;
    hal_gpio_port_t port;
    hal_gpio_interrupt_trigger_t trigger;
    uint8_t configured;
    uint8_t enabled;
} exti_line_t;

typedef struct {
    uint32_t priority;
    uint16_t registered;
    uint16_t enabled;
} exti_group_t;

static exti_line_t lines[EXTI_LINE_COUNT];
static exti_group_t groups[EXTI_GROUP_COUNT];

static int valid_pin(hal_gpio_port_t port, uint8_t pin)
{
    return port >= HAL_GPIO_PORT_0 && port < HAL_GPIO_PORT_COUNT && pin < EXTI_LINE_COUNT;
}

static uint32_t group_for(uint8_t pin)
{
    return pin < 5u ? pin : (pin < 10u ? 5u : 6u);
}

static uint32_t source_for(uint32_t group)
{
    if (group < 5u)
        return CH32V103_IRQ_EXTI0 + group;
    return group == 5u ? CH32V103_IRQ_EXTI9_5 : CH32V103_IRQ_EXTI15_10;
}

static uint32_t group_mask(uint32_t group)
{
    return group < 5u ? (1u << group) : (group == 5u ? 0x03E0u : 0xFC00u);
}

static void clear_pending(uint32_t mask)
{
    /* EXTI pending flags are W1C. */
    ch32v103_write32(EXTI_INTFR, mask);
    ch32v103_io_fence();
}

static void disable_line(uint8_t pin)
{
    uint32_t group = group_for(pin);
    uint32_t bit = 1u << pin;
    ch32v103_write32(EXTI_INTENR, ch32v103_read32(EXTI_INTENR) & ~bit);
    lines[pin].enabled = 0u;
    groups[group].enabled &= (uint16_t)~bit;
    clear_pending(bit);
    if (groups[group].registered != 0u && groups[group].enabled == 0u)
        (void)hal_interrupt_disable(source_for(group));
}

static void exti_dispatch(void *arg)
{
    uint32_t group = (uint32_t)(uintptr_t)arg;
    uint32_t pending = ch32v103_read32(EXTI_INTFR) &
                       ch32v103_read32(EXTI_INTENR) & group_mask(group);
    clear_pending(pending); /* clear before callbacks to preserve new edges */
    for (uint8_t pin = 0u; pin < EXTI_LINE_COUNT; ++pin) {
        hal_gpio_interrupt_handler_t handler = lines[pin].handler;
        if ((pending & (1u << pin)) != 0u && lines[pin].enabled != 0u && handler != NULL)
            handler(lines[pin].arg);
    }
}

ecos_err_t hal_gpio_interrupt_set_trigger(hal_gpio_port_t port, uint8_t pin,
                                           hal_gpio_interrupt_trigger_t trigger)
{
    uint32_t bit;
    uint32_t state;
    uint32_t shift;
    uint32_t route;
    if (!valid_pin(port, pin) || trigger < HAL_GPIO_INTERRUPT_DISABLE ||
        trigger > HAL_GPIO_INTERRUPT_HIGH_LEVEL)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (trigger == HAL_GPIO_INTERRUPT_LOW_LEVEL || trigger == HAL_GPIO_INTERRUPT_HIGH_LEVEL)
        return ECOS_ERR_UNSUPPORTED; /* CH32 EXTI supports edges, not levels */
    state = ch32v103_irq_lock();
    if (lines[pin].handler != NULL && lines[pin].port != port) {
        ch32v103_irq_restore(state);
        return ECOS_ERR_BUSY;
    }
    bit = 1u << pin;
    ch32v103_write32(RCC_APB2PCENR,
                     ch32v103_read32(RCC_APB2PCENR) | 1u | (1u << (2u + (uint32_t)port)));
    disable_line(pin);
    ch32v103_write32(EXTI_RTENR, ch32v103_read32(EXTI_RTENR) & ~bit);
    ch32v103_write32(EXTI_FTENR, ch32v103_read32(EXTI_FTENR) & ~bit);
    shift = (uint32_t)(pin & 3u) * 4u;
    route = ch32v103_read32(AFIO_EXTICR(pin >> 2));
    ch32v103_write32(AFIO_EXTICR(pin >> 2),
                     (route & ~(15u << shift)) | ((uint32_t)port << shift));
    if (trigger == HAL_GPIO_INTERRUPT_POSEDGE || trigger == HAL_GPIO_INTERRUPT_ANYEDGE)
        ch32v103_write32(EXTI_RTENR, ch32v103_read32(EXTI_RTENR) | bit);
    if (trigger == HAL_GPIO_INTERRUPT_NEGEDGE || trigger == HAL_GPIO_INTERRUPT_ANYEDGE)
        ch32v103_write32(EXTI_FTENR, ch32v103_read32(EXTI_FTENR) | bit);
    lines[pin].port = port;
    lines[pin].trigger = trigger;
    lines[pin].configured = 1u;
    ch32v103_irq_restore(state);
    return ECOS_OK;
}

ecos_err_t hal_gpio_interrupt_handler_add(hal_gpio_port_t port, uint8_t pin,
                                           hal_gpio_interrupt_handler_t handler,
                                           void *arg, uint32_t priority)
{
    uint32_t state;
    uint32_t group;
    ecos_err_t result;
    if (!valid_pin(port, pin) || handler == NULL || priority == 0u ||
        priority > CH32V103_IRQ_PRIORITY_MAX)
        return ECOS_ERR_INVALID_ARGUMENT;
    state = ch32v103_irq_lock();
    if (lines[pin].handler != NULL) {
        ch32v103_irq_restore(state);
        return ECOS_ERR_BUSY;
    }
    if (lines[pin].configured == 0u || lines[pin].port != port ||
        lines[pin].trigger == HAL_GPIO_INTERRUPT_DISABLE) {
        ch32v103_irq_restore(state);
        return ECOS_ERR_NOT_INITIALIZED;
    }
    group = group_for(pin);
    if (groups[group].registered != 0u && groups[group].priority != priority) {
        ch32v103_irq_restore(state);
        return ECOS_ERR_BUSY;
    }
    if (groups[group].registered == 0u) {
        result = hal_interrupt_alloc(source_for(group), priority, exti_dispatch,
                                      (void *)(uintptr_t)group);
        if (result != ECOS_OK) {
            ch32v103_irq_restore(state);
            return result;
        }
        groups[group].priority = priority;
    }
    lines[pin].handler = handler;
    lines[pin].arg = arg;
    groups[group].registered |= (uint16_t)(1u << pin);
    ch32v103_irq_restore(state);
    return ECOS_OK;
}

ecos_err_t hal_gpio_interrupt_handler_remove(hal_gpio_port_t port, uint8_t pin)
{
    uint32_t state;
    uint32_t group;
    ecos_err_t result = ECOS_OK;
    if (!valid_pin(port, pin))
        return ECOS_ERR_INVALID_ARGUMENT;
    state = ch32v103_irq_lock();
    if (lines[pin].handler == NULL || lines[pin].port != port) {
        ch32v103_irq_restore(state);
        return ECOS_ERR_NOT_INITIALIZED;
    }
    group = group_for(pin);
    disable_line(pin);
    groups[group].registered &= (uint16_t)~(1u << pin);
    lines[pin].handler = NULL;
    lines[pin].arg = NULL;
    if (groups[group].registered == 0u) {
        result = hal_interrupt_free(source_for(group));
        groups[group].priority = 0u;
    }
    ch32v103_irq_restore(state);
    return result;
}

ecos_err_t hal_gpio_interrupt_enable(hal_gpio_port_t port, uint8_t pin)
{
    uint32_t state;
    uint32_t bit;
    uint32_t group;
    ecos_err_t result;
    if (!valid_pin(port, pin))
        return ECOS_ERR_INVALID_ARGUMENT;
    state = ch32v103_irq_lock();
    if (lines[pin].handler == NULL || lines[pin].port != port ||
        lines[pin].trigger == HAL_GPIO_INTERRUPT_DISABLE) {
        ch32v103_irq_restore(state);
        return ECOS_ERR_NOT_INITIALIZED;
    }
    group = group_for(pin);
    bit = 1u << pin;
    clear_pending(bit);
    result = hal_interrupt_enable(source_for(group));
    if (result == ECOS_OK) {
        lines[pin].enabled = 1u;
        groups[group].enabled |= (uint16_t)bit;
        ch32v103_write32(EXTI_INTENR, ch32v103_read32(EXTI_INTENR) | bit);
        ch32v103_io_fence();
    }
    ch32v103_irq_restore(state);
    return result;
}

ecos_err_t hal_gpio_interrupt_disable(hal_gpio_port_t port, uint8_t pin)
{
    uint32_t state;
    if (!valid_pin(port, pin))
        return ECOS_ERR_INVALID_ARGUMENT;
    state = ch32v103_irq_lock();
    if (lines[pin].configured == 0u || lines[pin].port != port) {
        ch32v103_irq_restore(state);
        return ECOS_ERR_NOT_INITIALIZED;
    }
    disable_line(pin);
    ch32v103_irq_restore(state);
    return ECOS_OK;
}
