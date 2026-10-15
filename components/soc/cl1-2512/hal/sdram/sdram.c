#include "ecos/hal/sdram.h"
#include "cl1_2512_soc.h"

#include <stdint.h>

#define SDRAM_COMMAND_NOP       0x0000000Bu
#define SDRAM_COMMAND_PRECHARGE 0x00000009u
#define SDRAM_COMMAND_MODE      0x0000000Au
#define SDRAM_REFRESH_INITIAL   0x0000000Au
#define SDRAM_REFRESH_NORMAL    0x00000320u
#define SDRAM_CONTROL_NORMAL    0x01F8000Au

static void sdram_delay(uint32_t count)
{
    volatile uint32_t remaining = count;
    while (remaining != 0u) {
        --remaining;
        __asm__ volatile("nop");
    }
}

ecos_err_t hal_sdram_init(void)
{
    uint32_t readback;

    if (REG_SDRAM_CONTROL == SDRAM_CONTROL_NORMAL)
        return ECOS_OK;

    REG_SDRAM_COMMAND = SDRAM_COMMAND_NOP;
    readback = REG_SDRAM_COMMAND;
    (void)readback;
    sdram_delay(100u);
    REG_SDRAM_COMMAND = SDRAM_COMMAND_NOP;
    readback = REG_SDRAM_COMMAND;
    (void)readback;
    sdram_delay(200u);
    REG_SDRAM_COMMAND = SDRAM_COMMAND_PRECHARGE;
    readback = REG_SDRAM_COMMAND;
    (void)readback;
    REG_SDRAM_REFRESH = SDRAM_REFRESH_INITIAL;
    readback = REG_SDRAM_REFRESH;
    (void)readback;
    sdram_delay(9u);
    REG_SDRAM_REFRESH = SDRAM_REFRESH_NORMAL;
    readback = REG_SDRAM_REFRESH;
    (void)readback;
    REG_SDRAM_COMMAND = SDRAM_COMMAND_MODE;
    readback = REG_SDRAM_COMMAND;
    (void)readback;
    __asm__ volatile("fence rw, rw" : : : "memory");
    readback = REG_SDRAM_MODE_ACCESS;
    (void)readback;
    __asm__ volatile("fence rw, rw" : : : "memory");
    REG_SDRAM_CONTROL = SDRAM_CONTROL_NORMAL;
    readback = REG_SDRAM_CONTROL;
    REG_SDRAM_COMMAND = 0u;
    (void)REG_SDRAM_COMMAND;
    __asm__ volatile("fence rw, rw" : : : "memory");
    return readback == SDRAM_CONTROL_NORMAL ? ECOS_OK : ECOS_ERR_IO;
}
