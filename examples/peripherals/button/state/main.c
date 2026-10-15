#include "ecos/bsp/button.h"
#include "ecos/bsp/console.h"
#include "ecos/log.h"

#define LOG_TAG "button-state"

int main(void)
{
    int previous[BSP_BUTTON_COUNT];
    int count;

    ECOS_PANIC_ON_ERROR(LOG_TAG, bsp_console_init(), "initialize console");
    ECOS_PANIC_ON_ERROR(LOG_TAG, bsp_button_init(), "initialize buttons");
    count = bsp_button_get_count();
    if (count <= 0 || count > BSP_BUTTON_COUNT)
        ECOS_PANIC(LOG_TAG, "invalid Board button count: %d", count);
    for (int button = 0; button < count; ++button)
        previous[button] = -1;

    for (;;) {
        for (int button = 0; button < count; ++button) {
            int state = bsp_button_get_state((bsp_button_t)button);

            ECOS_PANIC_ON_ERROR(LOG_TAG, state, "read button");
            if (state != previous[button]) {
                (void)ECOS_LOGI(LOG_TAG, "button %d: %s", button,
                                state == BSP_BUTTON_PRESSED ? "pressed" : "released");
                previous[button] = state;
            }
        }
    }
}
