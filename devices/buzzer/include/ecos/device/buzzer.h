#ifndef ECOS_DEVICE_BUZZER_H
#define ECOS_DEVICE_BUZZER_H

#include "ecos/driver/pwm.h"
#include "ecos/error.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    ecos_pwm_id_t pwm;
    ecos_pwm_channel_t channel;
    /* Input clock of the PWM controller, used to derive tone timings. */
    uint32_t pwm_clock_hz;
} ecos_buzzer_config_t;

typedef struct {
    ecos_buzzer_config_t config;
    uint8_t initialized;
} ecos_buzzer_t;

/* StarrySky L4 on-board buzzer: 4 kHz passive buzzer driven by an S8050
 * from PWM0 channel 0 on GPIO1[14]; the SoC PWM counts at CPU clock. */
#define ECOS_BUZZER_CONFIG_DEFAULT \
    { ECOS_PWM_DEFAULT, ECOS_PWM_CHANNEL_0, 50000000u }

/* Stores the configuration; the buzzer stays silent until a tone starts. */
ecos_err_t ecos_buzzer_init(ecos_buzzer_t *buzzer,
                            const ecos_buzzer_config_t *config);
ecos_err_t ecos_buzzer_deinit(ecos_buzzer_t *buzzer);

/* Starts a continuous tone at frequency_hz with a 50 percent duty cycle. */
ecos_err_t ecos_buzzer_play_tone(ecos_buzzer_t *buzzer, uint32_t frequency_hz);

/* Blocking helper: tone for duration_ms, then silence. */
ecos_err_t ecos_buzzer_beep(ecos_buzzer_t *buzzer,
                            uint32_t frequency_hz,
                            uint32_t duration_ms);

ecos_err_t ecos_buzzer_stop(ecos_buzzer_t *buzzer);

#ifdef __cplusplus
}
#endif

#endif
