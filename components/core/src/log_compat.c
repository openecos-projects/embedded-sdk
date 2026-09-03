#include "log.h"

#include <stdarg.h>

void log_init(LogLevel level, const char *filename)
{
    (void)filename;
    (void)ecos_log_set_level(level);
}

void log_print(LogLevel level,
               const char *file,
               int line,
               const char *format,
               ...)
{
    va_list arguments;

    va_start(arguments, format);
    (void)ecos_log_vwrite(level, "legacy", file, line, format, arguments);
    va_end(arguments);
}

void log_close(void)
{
}
