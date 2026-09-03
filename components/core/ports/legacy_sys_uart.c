#include "ecos/error.h"

#include "hal_sys_uart.h"

#include <limits.h>
#include <stddef.h>

int ecos_log_platform_write(void *context, const char *data, size_t size)
{
    size_t index;

    (void)context;
    if ((data == (const char *)0 && size != 0u) || size > INT_MAX)
        return ECOS_ERR_INVALID_ARGUMENT;
    for (index = 0u; index < size; ++index)
        hal_sys_putchar(data[index]);
    return (int)size;
}
