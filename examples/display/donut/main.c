#include "ecos/bsp/console.h"
#include "ecos/bsp/display.h"
#include "ecos/driver/timer.h"
#include "ecos/log.h"

#include <stdint.h>

#define LOG_TAG "donut"
#define GRID_SIZE 16u

static const int16_t sine[16] = {
    0, 98, 181, 237, 256, 237, 181, 98,
    0, -98, -181, -237, -256, -237, -181, -98,
};

static const uint16_t shades[10] = {
    0x0861u, 0x10C2u, 0x1923u, 0x21A4u, 0x2A25u,
    0x32A6u, 0x3B27u, 0x4BC9u, 0x648Bu, 0x854Du,
};

static uint16_t pixels[GRID_SIZE * GRID_SIZE];
static int16_t depths[GRID_SIZE * GRID_SIZE];

static int32_t cosine(uint8_t phase)
{
    return sine[(phase + 4u) & 15u];
}

static void render(uint8_t pitch, uint8_t yaw)
{
    uint32_t index;
    uint8_t u;
    uint8_t v;
    int32_t cp = cosine(pitch);
    int32_t sp = sine[pitch];
    int32_t cy = cosine(yaw);
    int32_t sy = sine[yaw];

    for (index = 0u; index < GRID_SIZE * GRID_SIZE; ++index) {
        pixels[index] = 0x0000u;
        depths[index] = 32767;
    }

    for (u = 0u; u < 16u; ++u) {
        for (v = 0u; v < 16u; ++v) {
            int32_t radius = 470 + (180 * cosine(v) >> 8);
            int32_t x = radius * cosine(u) >> 8;
            int32_t y = radius * sine[u] >> 8;
            int32_t z = 180 * sine[v] >> 8;
            int32_t rotated_y = (y * cp - z * sp) >> 8;
            int32_t rotated_z = (y * sp + z * cp) >> 8;
            int32_t rotated_x = (x * cy + rotated_z * sy) >> 8;
            int32_t depth = 1200 + ((rotated_z * cy - x * sy) >> 8);
            int32_t screen_x = 8 + (rotated_x * 13) / depth;
            int32_t screen_y = 8 + (rotated_y * 13) / depth;
            int32_t shade;

            if (screen_x < 0 || screen_x >= GRID_SIZE ||
                screen_y < 0 || screen_y >= GRID_SIZE)
                continue;
            index = (uint32_t)screen_y * GRID_SIZE + (uint32_t)screen_x;
            if (depth >= depths[index])
                continue;
            depths[index] = (int16_t)depth;
            shade = (1800 - depth) / 100;
            if (shade < 0)
                shade = 0;
            if (shade > 9)
                shade = 9;
            pixels[index] = shades[shade];
        }
    }
}

int main(void)
{
    uint16_t tile_width;
    uint16_t tile_height;
    uint8_t pitch = 0u;
    uint8_t yaw = 0u;

    ECOS_PANIC_ON_ERROR(LOG_TAG, bsp_console_init(), "initialize console");
    ECOS_PANIC_ON_ERROR(LOG_TAG, bsp_display_init(), "initialize display");
    tile_width = bsp_display_width() / GRID_SIZE;
    tile_height = bsp_display_height() / GRID_SIZE;
    if (tile_width == 0u || tile_height == 0u)
        ECOS_PANIC_ON_ERROR(LOG_TAG, ECOS_ERR_INVALID_ARGUMENT, "display size");

    for (;;) {
        uint16_t row;
        uint16_t column;
        render(pitch, yaw);
        for (row = 0u; row < GRID_SIZE; ++row) {
            for (column = 0u; column < GRID_SIZE; ++column) {
                ECOS_PANIC_ON_ERROR(
                    LOG_TAG,
                    bsp_display_fill_rect(
                        column * tile_width, row * tile_height,
                        tile_width, tile_height,
                        pixels[row * GRID_SIZE + column]
                    ),
                    "draw donut tile"
                );
            }
        }
        pitch = (pitch + 1u) & 15u;
        yaw = (yaw + 2u) & 15u;
        ECOS_PANIC_ON_ERROR(
            LOG_TAG, ecos_timer_delay_ms(ECOS_TIMER_DEFAULT, 80u), "next frame"
        );
    }
}
