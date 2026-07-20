#include "hal_timer.h"
#include "generated/autoconf.h"
#include "board.h"

// 提供一个弱定义的延时函数，供 devices 外设驱动默认调用。
// 用户可以在自己的应用代码（如 main.c）中重写这个函数，提供更精准的实现。
__attribute__((weak)) void delay_ms(uint32_t val) {
    hal_delay_ms(0, val);
}

typedef struct {
    volatile uint32_t *ctrl;
    volatile uint32_t *pscr;
    volatile uint32_t *cnt;
    volatile uint32_t *cmp;
    volatile uint32_t *stat;
} timer_regs_t;

static uint8_t timer_get_regs(uint8_t timer_id, timer_regs_t *regs)
{
    switch (timer_id) {
    case 0:
        regs->ctrl = &REG_TIMER_0_CTRL;
        regs->pscr = &REG_TIMER_0_PSCR;
        regs->cnt = &REG_TIMER_0_CNT;
        regs->cmp = &REG_TIMER_0_CMP;
        regs->stat = &REG_TIMER_0_STAT;
        return 0;
    case 1:
        regs->ctrl = &REG_TIMER_1_CTRL;
        regs->pscr = &REG_TIMER_1_PSCR;
        regs->cnt = &REG_TIMER_1_CNT;
        regs->cmp = &REG_TIMER_1_CMP;
        regs->stat = &REG_TIMER_1_STAT;
        return 0;
    case 2:
        regs->ctrl = &REG_TIMER_2_CTRL;
        regs->pscr = &REG_TIMER_2_PSCR;
        regs->cnt = &REG_TIMER_2_CNT;
        regs->cmp = &REG_TIMER_2_CMP;
        regs->stat = &REG_TIMER_2_STAT;
        return 0;
    case 3:
        regs->ctrl = &REG_TIMER_3_CTRL;
        regs->pscr = &REG_TIMER_3_PSCR;
        regs->cnt = &REG_TIMER_3_CNT;
        regs->cmp = &REG_TIMER_3_CMP;
        regs->stat = &REG_TIMER_3_STAT;
        return 0;
    default:
        return 1;
    }
}

uint8_t hal_delay_us(uint8_t timer_id, uint32_t val){
    timer_regs_t regs;

    if (val == 0u || timer_get_regs(timer_id, &regs) != 0u) {
        return 1;
    }

    *regs.ctrl = 0u;
    while (*regs.stat == 1u)
        ;

    *regs.pscr = CONFIG_TIMER_FREQ_MHZ - 1u;
    *regs.cmp = val - 1u;
    *regs.ctrl = 0xDu;

    while (*regs.stat == 0u)
        ;

    *regs.ctrl = 0u;
    while (*regs.stat == 1u)
        ;

    return 0;
}

uint8_t hal_delay_ms(uint8_t timer_id, uint32_t val){
    while (val-- != 0u) {
        if (hal_delay_us(timer_id, 1000u) != 0u) {
            return 1;
        }
    }

    return 0;
}

uint8_t hal_delay_s(uint8_t timer_id, uint32_t val){
    while (val-- != 0u) {
        if (hal_delay_ms(timer_id, 1000u) != 0u) {
            return 1;
        }
    }

    return 0;
}

uint8_t hal_sys_tick_init(uint8_t timer_id){
    timer_regs_t regs;

    if (timer_get_regs(timer_id, &regs) != 0u) {
        return 1;
    }

    *regs.ctrl = 0u;
    while (*regs.stat == 1u)
        ;

    *regs.pscr = CONFIG_TIMER_FREQ_MHZ - 1u;
    *regs.cmp = 0xFFFFFFFFu;
    *regs.ctrl = 0xDu;

    return 0;
}

uint32_t hal_get_sys_tick(uint8_t timer_id){
    timer_regs_t regs;

    if (timer_get_regs(timer_id, &regs) != 0u) {
        return 0;
    }

    return *regs.cnt;
}
