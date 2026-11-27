/* CH32V103 PWM register model. A natural overflow or UG transfers the preloads. */
#include "ch32v103_interrupt_internal.h"
#include "ecos/driver/pwm.h"

#include <assert.h>
#include <stdio.h>

static const uintptr_t bases[] = {0x40012C00u, 0x40000400u};
enum { CTLR1 = 0, SWEVGR = 0x14, CHCTLR1 = 0x18, CHCTLR2 = 0x1c,
       CCER = 0x20, CNT = 0x24, PSC = 0x28, ATRLR = 0x2c, CH1CVR = 0x34, BDTR = 0x44 };

typedef struct {
    uint16_t reg[BDTR / 4 + 1];
    uint16_t active[4];
    uint16_t count;
    uint16_t reload;
    unsigned updates;
} timer_model_t;

static timer_model_t timers[2];
static unsigned irq_locked;

static struct {
    uintptr_t address;
    uint32_t value;
} shared[] = {
    {0x40021018u, 0x4000u}, {0x4002101cu, 0x200000u},
    {0x40010004u, 0x04000303u}, {0x40010804u, 0x44444444u}, {0x40010c00u, 0xff444444u},
};

uint32_t ch32v103_irq_lock(void)
{
    assert(!irq_locked);
    irq_locked = 1;
    return 0;
}

void ch32v103_irq_restore(uint32_t state)
{
    assert(irq_locked && state == 0);
    irq_locked = 0;
}

static uint32_t *shared_register(uintptr_t addr)
{
    for (unsigned i = 0; i < sizeof(shared) / sizeof(shared[0]); ++i)
        if (addr == shared[i].address)
            return &shared[i].value;
    assert(0 && "unexpected GPIO/RCC/AFIO register");
    return NULL;
}

uint32_t ch32v103_read32(uintptr_t addr) { return *shared_register(addr); }
void ch32v103_write32(uintptr_t addr, uint32_t value) { *shared_register(addr) = value; }

static unsigned timer_index(uintptr_t addr)
{
    unsigned id = addr >= bases[0] ? 0 : 1;
    assert(addr >= bases[id] && addr <= bases[id] + BDTR && addr % 4 == 0);
    assert(id == 0 || addr != bases[id] + BDTR); /* No main-output gate on TIM3. */
    return id;
}

uint16_t ch32v103_read16(uintptr_t addr)
{
    unsigned id = timer_index(addr);
    return timers[id].reg[(addr - bases[id]) / 4];
}

static void update(timer_model_t *timer)
{
    for (unsigned i = 0; i < 4; ++i)
        timer->active[i] = timer->reg[CH1CVR / 4 + i];
    timer->reload = timer->reg[ATRLR / 4];
    timer->count = 0;
    ++timer->updates;
}

void ch32v103_write16(uintptr_t addr, uint16_t value)
{
    unsigned id = timer_index(addr);
    timer_model_t *timer = &timers[id];
    unsigned offset = (unsigned)(addr - bases[id]);
    assert(irq_locked);
    if (offset == SWEVGR) {
        assert(value == 1);
        update(timer);
    } else if (offset == CNT) {
        timer->count = value;
    } else {
        timer->reg[offset / 4] = value;
        if (offset >= CH1CVR && offset < CH1CVR + 16) {
            unsigned ch = (offset - CH1CVR) / 4;
            unsigned mode = timer->reg[CHCTLR1 / 4 + ch / 2] >> (8 * (ch % 2));
            if (!(mode & 0x08)) /* OCxPE off: immediate write to active compare. */
                timer->active[ch] = value;
        }
    }
}

/* Count output high times over a span of prescaled timer ticks. */
static void sample(timer_model_t *timer, unsigned ticks, unsigned high[4])
{
    assert(timer->reg[CTLR1 / 4] & 1);
    while (ticks-- > 0) {
        for (unsigned i = 0; i < 4; ++i) {
            unsigned field = timer->reg[CHCTLR1 / 4 + i / 2] >> (8 * (i % 2));
            unsigned mode = (field >> 4) & 7u;
            if ((timer->reg[CCER / 4] & (1u << (4 * i))) &&
                (mode == 5 || (mode == 6 && timer->count < timer->active[i])))
                ++high[i];
        }
        if (timer->reload == 0u)
            continue; /* ARR=0 stops the counter: there is no natural update. */
        if (timer->count == timer->reload)
            update(timer);
        else
            ++timer->count;
    }
}

static void assert_pulses(unsigned a, unsigned b, unsigned c, unsigned d)
{
    assert(timers[1].active[0] == a && timers[1].active[1] == b);
    assert(timers[1].active[2] == c && timers[1].active[3] == d);
}

static void test_single_tick_period(ecos_pwm_id_t id, ecos_pwm_channel_t channel)
{
    const ecos_pwm_config_t single_tick = {8u, 1u};
    timer_model_t *timer = &timers[id];
    unsigned high[4] = {0};
    assert(ecos_pwm_init(id, &single_tick) == ECOS_OK);
    assert(ecos_pwm_start(id) == ECOS_OK);
    unsigned events = timer->updates;
    assert(ecos_pwm_set_duty_cycle(id, channel, 100u) == ECOS_OK);
    sample(timer, 5, high);
    assert(high[channel] == 5);
    assert(ecos_pwm_set_pulse_ticks(id, channel, 0u) == ECOS_OK);
    sample(timer, 5, high);
    assert(high[channel] == 5);
    assert(ecos_pwm_set_pulse_ticks(id, channel, 1u) == ECOS_OK);
    sample(timer, 5, high);
    assert(high[channel] == 10);
    for (unsigned i = 0; i < 4; ++i)
        if (i != (unsigned)channel)
            assert(high[i] == 0);
    assert(timer->updates == events);
}

int main(void)
{
    const ecos_pwm_config_t servo = {8, 20000}, maximum = {65536, 65536};
    const uint32_t initial[] = {1000, 1490, 1500, 2000};
    timer_model_t *tim3 = &timers[1];
    assert(ecos_pwm_get_instance_count() == 2);
    assert(ecos_pwm_start(1) == ECOS_ERR_NOT_INITIALIZED);
    assert(ecos_pwm_set_pulse_ticks(1, ECOS_PWM_CHANNEL_1, 1) == ECOS_ERR_NOT_INITIALIZED);
    assert(ecos_pwm_init(2, &servo) == ECOS_ERR_INVALID_ARGUMENT);
    assert(ecos_pwm_init(1, &(ecos_pwm_config_t){8, 65537}) == ECOS_ERR_INVALID_ARGUMENT);
    assert(ecos_pwm_init(1, &(ecos_pwm_config_t){65537, 1}) == ECOS_ERR_INVALID_ARGUMENT);
    assert(ecos_pwm_init(0, &maximum) == ECOS_OK);
    assert(ecos_pwm_set_duty_cycle(0, ECOS_PWM_CHANNEL_0, 25) == ECOS_OK);
    assert(ecos_pwm_start(0) == ECOS_OK);
    assert(timers[0].active[0] == 16384 && timers[0].reg[BDTR / 4] == 0x8000);
    assert(ecos_pwm_init(1, &servo) == ECOS_OK);
    assert(ch32v103_read32(0x40010004u) == 0x04000b03u);
    assert(ch32v103_read32(0x40010804u) == 0x4444444bu);
    assert(ch32v103_read32(0x40010c00u) == 0xffbb44bbu);
    assert(ch32v103_read32(0x40021018u) == 0x480du);
    assert(ch32v103_read32(0x4002101cu) == 0x200002u);
    assert(tim3->reg[PSC / 4] == 7 && tim3->reg[ATRLR / 4] == 19999);
    assert(tim3->reg[CHCTLR1 / 4] == 0x6868 && tim3->reg[CHCTLR2 / 4] == 0x6868);
    for (unsigned i = 0; i < 4; ++i)
        assert(ecos_pwm_set_pulse_ticks(1, (ecos_pwm_channel_t)i, initial[i]) == ECOS_OK);
    assert(!(tim3->reg[CTLR1 / 4] & 1) && tim3->reg[CCER / 4] == 0);
    assert(ecos_pwm_start(1) == ECOS_OK);
    assert((tim3->reg[CTLR1 / 4] & 1) && tim3->reg[CCER / 4] == 0x1111);
    assert_pulses(1000, 1490, 1500, 2000); /* First frame already contains all four values. */

    unsigned high[4] = {0};
    sample(tim3, 1450, high); /* During the second channel's high pulse. */
    unsigned events = tim3->updates;
    assert(ecos_pwm_set_pulse_ticks(1, ECOS_PWM_CHANNEL_1, 1510) == ECOS_OK);
    assert(ecos_pwm_set_pulse_ticks(1, ECOS_PWM_CHANNEL_1, 20001) == ECOS_ERR_INVALID_ARGUMENT);
    assert(ecos_pwm_set_pulse_ticks(1, ECOS_PWM_CHANNEL_COUNT, 10) == ECOS_ERR_INVALID_ARGUMENT);
    assert(ecos_pwm_set_duty_cycle(1, ECOS_PWM_CHANNEL_0, 101) == ECOS_ERR_INVALID_ARGUMENT);
    assert(ecos_pwm_set_pulse_ticks(0, ECOS_PWM_CHANNEL_1, 1) == ECOS_ERR_INVALID_ARGUMENT);
    assert(ecos_pwm_start(1) == ECOS_OK);
    assert(ecos_pwm_stop(1) == ECOS_OK && !(tim3->reg[CTLR1 / 4] & 1));
    assert(ecos_pwm_start(1) == ECOS_OK && (tim3->reg[CTLR1 / 4] & 1));
    assert(tim3->count == 1450 && tim3->updates == events);
    assert_pulses(1000, 1490, 1500, 2000);
    sample(tim3, 20000 - 1450, high);
    assert(high[0] == 1000 && high[1] == 1490 && high[2] == 1500 && high[3] == 2000);
    assert(tim3->count == 0 && tim3->updates == events + 1);
    assert_pulses(1000, 1510, 1500, 2000);
    unsigned next_high[4] = {0};
    sample(tim3, 20000, next_high);
    assert(next_high[0] == 1000 && next_high[1] == 1510 &&
           next_high[2] == 1500 && next_high[3] == 2000);
    assert(ecos_pwm_set_pulse_ticks(1, ECOS_PWM_CHANNEL_0, 20000) == ECOS_OK);
    assert(ecos_pwm_set_duty_cycle(1, ECOS_PWM_CHANNEL_3, 0) == ECOS_OK);
    update(tim3);
    assert_pulses(20000, 1510, 1500, 0);
    assert(timers[0].active[0] == 16384 && (timers[0].reg[CTLR1 / 4] & 1));

    assert(ecos_pwm_init(1, &maximum) == ECOS_OK);
    assert(ecos_pwm_start(1) == ECOS_OK);
    assert_pulses(0, 0, 0, 0);
    assert(tim3->reg[PSC / 4] == 65535 && tim3->reg[ATRLR / 4] == 65535);
    for (unsigned i = 0; i < 4; ++i) {
        ecos_pwm_channel_t ch = (ecos_pwm_channel_t)i;
        assert(ecos_pwm_set_pulse_ticks(1, ch, 65536) == ECOS_OK);
        assert(tim3->reg[CHCTLR1 / 4 + i / 2] == (i % 2 ? 0x5868 : 0x6858));
        assert(tim3->reg[CHCTLR1 / 4 + (1 - i / 2)] == 0x6868);
        unsigned maximum_high[4] = {0};
        sample(tim3, 65536, maximum_high);
        assert(maximum_high[i] == 65536);
        assert(ecos_pwm_set_duty_cycle(1, ch, 7) == ECOS_OK);
        assert(tim3->reg[CHCTLR1 / 4 + i / 2] == 0x6868);
        update(tim3);
        assert(tim3->active[i] == 4587);
    }
    assert(ecos_pwm_set_duty_cycle(0, ECOS_PWM_CHANNEL_0, 100) == ECOS_OK);
    assert(timers[0].reg[CHCTLR1 / 4] == 0x58);
    assert(ecos_pwm_set_pulse_ticks(0, ECOS_PWM_CHANNEL_0, 1500) == ECOS_OK);
    assert(timers[0].reg[CHCTLR1 / 4] == 0x68);
    update(&timers[0]);
    assert(timers[0].active[0] == 1500);
    test_single_tick_period(0u, ECOS_PWM_CHANNEL_0);
    test_single_tick_period(1u, ECOS_PWM_CHANNEL_3);
    assert(!irq_locked);
    puts("TIM1/TIM3 PWM register tests passed");
    return 0;
}
