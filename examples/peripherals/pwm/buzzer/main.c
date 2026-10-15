#include "ecos/bsp/console.h"
#include "ecos/device/buzzer.h"
#include "ecos/driver/timer.h"
#include "ecos/log.h"

#include <stddef.h>
#include <stdint.h>

#define LOG_TAG "pwm-buzzer"
#define RATED_TONE_HZ 4000u
#define SCALE_NOTE_MS 250u
#define MELODY_PAUSE_MS 1000u

typedef struct {
    uint32_t frequency_hz;
    uint32_t duration_ms;
} note_t;

/* C major scale, one octave. */
static const note_t scale[] = {
    { 523u, SCALE_NOTE_MS },  /* C5 */
    { 587u, SCALE_NOTE_MS },  /* D5 */
    { 659u, SCALE_NOTE_MS },  /* E5 */
    { 698u, SCALE_NOTE_MS },  /* F5 */
    { 784u, SCALE_NOTE_MS },  /* G5 */
    { 880u, SCALE_NOTE_MS },  /* A5 */
    { 988u, SCALE_NOTE_MS },  /* B5 */
    { 1047u, SCALE_NOTE_MS }, /* C6 */
};

static void halt(void)
{
    for (;;)
        __asm__ volatile("nop");
}

static void play(const note_t *notes, size_t count, ecos_buzzer_t *buzzer)
{
    size_t index;

    for (index = 0u; index < count; ++index)
        ECOS_PANIC_ON_ERROR(
            LOG_TAG,
            ecos_buzzer_beep(
                buzzer, notes[index].frequency_hz, notes[index].duration_ms
            ),
            "play note"
        );
}

int main(void)
{
    const ecos_buzzer_config_t buzzer_config = ECOS_BUZZER_CONFIG_DEFAULT;
    ecos_buzzer_t buzzer;

    ECOS_PANIC_ON_ERROR(
        LOG_TAG, bsp_console_init(), "initialize console"
    );
    ECOS_PANIC_ON_ERROR(
        LOG_TAG,
        ecos_buzzer_init(&buzzer, &buzzer_config),
        "initialize buzzer"
    );

    (void)ECOS_LOGI(LOG_TAG, "buzzer ready, rated tone %u Hz", RATED_TONE_HZ);

    for (;;) {
        ECOS_PANIC_ON_ERROR(
            LOG_TAG,
            ecos_buzzer_beep(&buzzer, RATED_TONE_HZ, 500u),
            "beep at rated frequency"
        );
        ECOS_PANIC_ON_ERROR(
            LOG_TAG,
            ecos_timer_delay_ms(ECOS_TIMER_DEFAULT, MELODY_PAUSE_MS),
            "pause after rated tone"
        );

        (void)ECOS_LOGI(LOG_TAG, "playing scale");
        play(scale, sizeof(scale) / sizeof(scale[0]), &buzzer);
        ECOS_PANIC_ON_ERROR(
            LOG_TAG,
            ecos_timer_delay_ms(ECOS_TIMER_DEFAULT, MELODY_PAUSE_MS),
            "pause after scale"
        );
    }

    halt();
}
