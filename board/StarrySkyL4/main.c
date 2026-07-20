#include "main.h"

#include "hal_archinfo.h"
#include "hal_crc.h"
#include "hal_gpio.h"
#include "hal_hp_uart.h"
#include "hal_pwm.h"
#include "hal_qspi.h"
#include "hal_rng.h"
#include "hal_sys_uart.h"
#include "hal_timer.h"

#define GPIO0_TEST_PINS     0xFFFFFF00u
#define GPIO1_TEST_PINS     0x003FFFFFu
#define GPIO2_TEST_PINS     0x00000FFFu

#define PWM_PERIOD_TICKS    1000u
#define PWM_PRESCALE        100u
#define HP_UART_BAUDRATE    115200u

static void putstr(const char *str)
{
    hal_sys_putstr((char *)str);
}

static void put_hex32(uint32_t val)
{
    static const char hex[] = "0123456789ABCDEF";
    char buf[] = "0x00000000";

    for (uint32_t i = 0u; i < 8u; i++) {
        uint32_t shift = (7u - i) * 4u;
        buf[2u + i] = hex[(val >> shift) & 0x0Fu];
    }

    putstr(buf);
}

static void delay_cycles(uint32_t cycles)
{
    volatile uint32_t count = cycles;

    while (count-- != 0u)
        ;
}

void delay_ms(uint32_t val)
{
    (void)hal_delay_ms(0, val);
}

static void archinfo_smoke(void)
{
    putstr("[L4] ARCHINFO SYS=");
    put_hex32(hal_archinfo_get_sys());
    putstr(" IDL=");
    put_hex32(hal_archinfo_get_idl());
    putstr(" IDH=");
    put_hex32(hal_archinfo_get_idh());
    putstr("\n");
}

static void crc_smoke(void)
{
    putstr("[L4] CRC smoke\n");

    hal_crc_set_ctrl(0u);
    hal_crc_set_init(0x0000FFFFu);
    hal_crc_set_xorv(0u);
    hal_crc_set_ctrl((1u << 0) | (1u << 3) | (2u << 5));
    hal_crc_set_data(0x00123456u);
    delay_cycles(8u);

    putstr("  CRC DATA=");
    put_hex32(hal_crc_get_val());
    putstr("\n");
}

static void gpio_smoke(void)
{
    putstr("[L4] GPIO smoke\n");

    REG_GPIO_0_IOFCFG &= ~GPIO0_TEST_PINS;
    REG_GPIO_0_PINMUX &= ~GPIO0_TEST_PINS;
    REG_GPIO_0_PADDIR |= GPIO0_TEST_PINS;
    REG_GPIO_0_PADOUT ^= GPIO0_TEST_PINS;

    REG_GPIO_1_IOFCFG &= ~GPIO1_TEST_PINS;
    REG_GPIO_1_PINMUX &= ~GPIO1_TEST_PINS;
    REG_GPIO_1_PADDIR |= GPIO1_TEST_PINS;
    REG_GPIO_1_PADOUT ^= GPIO1_TEST_PINS;

    REG_GPIO_2_IOFCFG &= ~GPIO2_TEST_PINS;
    REG_GPIO_2_PINMUX &= ~GPIO2_TEST_PINS;
    REG_GPIO_2_PADDIR |= GPIO2_TEST_PINS;
    REG_GPIO_2_PADOUT ^= GPIO2_TEST_PINS;

    putstr("  GPIO toggled\n");
}

static void hp_uart_smoke(void)
{
    putstr("[L4] HP UART smoke\n");
    hal_hp_uart_init(HP_UART_BAUDRATE);
    hal_hp_uart_putstr("L4 UART1 TX smoke\n");
}

static void pwm_smoke(void)
{
    pwm_config_t pwm_cfg = {
        .pscr = PWM_PRESCALE,
        .cmp = PWM_PERIOD_TICKS,
    };

    putstr("[L4] PWM smoke\n");

    (void)pwm_hal_init(0, 0, &pwm_cfg);
    (void)pwm_hal_init(0, 1, &pwm_cfg);

    for (pwm_channel_t ch = PWM_CH0; ch < PWM_CH_MAX; ch++) {
        (void)pwm_hal_set_compare(0, 0, ch, PWM_PERIOD_TICKS / 2u);
        (void)pwm_hal_set_compare(0, 1, ch, PWM_PERIOD_TICKS / 2u);
    }

    (void)pwm_hal_enable(0, 0);
    (void)pwm_hal_enable(0, 1);
    delay_cycles(10000u);
    (void)pwm_hal_disable(0, 0);
    (void)pwm_hal_disable(0, 1);
}

static void qspi_smoke(void)
{
    hal_qspi_config_t qspi_cfg = {
        .clkdiv = 4u,
    };

    putstr("[L4] QSPI init smoke\n");
    if (hal_qspi_init(HAL_QSPI_PORT_0, &qspi_cfg) == 0) {
        putstr("  QSPI init ok\n");
    } else {
        putstr("  QSPI init failed\n");
    }
}

static void rng_smoke(void)
{
    putstr("[L4] RNG smoke\n");

    hal_rng_set_ctrl(1u);
    hal_rng_set_seed(0x0000FE1Cu);
    delay_cycles(32u);

    putstr("  RNG VAL=");
    put_hex32(hal_rng_get_val());
    putstr("\n");
}

static void timer_smoke(void)
{
    putstr("[L4] TIMER smoke\n");
    (void)hal_delay_us(0, 1000u);
    putstr("  TIMER delay ok\n");
}

int main(void)
{
    hal_sys_uart_init();
    putstr("\nStarrySkyL4 SDK smoke start\n");

    archinfo_smoke();
    crc_smoke();
    gpio_smoke();
    hp_uart_smoke();
    pwm_smoke();
    qspi_smoke();
    rng_smoke();
    timer_smoke();

    putstr("StarrySkyL4 SDK smoke done\n");

    while (1) {
        ;
    }

    return 0;
}
