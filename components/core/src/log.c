#include "ecos/log.h"

#include <stdint.h>

#ifndef CONFIG_ECOS_LOG_BUFFER_SIZE
#define CONFIG_ECOS_LOG_BUFFER_SIZE 256
#endif

#if CONFIG_ECOS_LOG_BUFFER_SIZE < 64
#error CONFIG_ECOS_LOG_BUFFER_SIZE must be at least 64 bytes
#endif

typedef struct {
    char *data;
    size_t capacity;
    size_t length;
    int truncated;
} log_buffer_t;

static ecos_log_writer_t log_writer;
static void *log_writer_context;
static ecos_panic_handler_t panic_handler;
static void *panic_handler_context;
static ecos_log_level_t current_log_level =
    (ecos_log_level_t)CONFIG_ECOS_LOG_LEVEL;

static const char level_characters[] = { 'D', 'I', 'W', 'E', 'F' };

#if defined(__GNUC__) || defined(__clang__)
__attribute__((weak))
#endif
int ecos_log_platform_write(void *context, const char *data, size_t size)
{
    (void)context;
    (void)data;
    (void)size;
    return ECOS_ERR_NOT_INITIALIZED;
}

static void buffer_append_char(log_buffer_t *buffer, char value)
{
    if (buffer->length + 1u < buffer->capacity) {
        buffer->data[buffer->length++] = value;
        buffer->data[buffer->length] = '\0';
    } else {
        buffer->truncated = 1;
    }
}

static void buffer_append_string(log_buffer_t *buffer, const char *value)
{
    const char *text = value != (const char *)0 ? value : "(null)";

    while (*text != '\0')
        buffer_append_char(buffer, *text++);
}

static void buffer_append_repeat(log_buffer_t *buffer, char value, unsigned count)
{
    while (count-- != 0u)
        buffer_append_char(buffer, value);
}

static size_t decimal_digits(uint64_t value, char *digits)
{
    static const uint64_t powers[] = {
        UINT64_C(10000000000000000000),
        UINT64_C(1000000000000000000),
        UINT64_C(100000000000000000),
        UINT64_C(10000000000000000),
        UINT64_C(1000000000000000),
        UINT64_C(100000000000000),
        UINT64_C(10000000000000),
        UINT64_C(1000000000000),
        UINT64_C(100000000000),
        UINT64_C(10000000000),
        UINT64_C(1000000000),
        UINT64_C(100000000),
        UINT64_C(10000000),
        UINT64_C(1000000),
        UINT64_C(100000),
        UINT64_C(10000),
        UINT64_C(1000),
        UINT64_C(100),
        UINT64_C(10),
        UINT64_C(1)
    };
    size_t count = 0u;
    size_t index;
    int started = 0;

    for (index = 0u; index < sizeof(powers) / sizeof(powers[0]); ++index) {
        unsigned digit = 0u;

        while (value >= powers[index]) {
            value -= powers[index];
            ++digit;
        }
        if (digit != 0u || started != 0 || powers[index] == UINT64_C(1)) {
            digits[count++] = (char)('0' + digit);
            started = 1;
        }
    }
    return count;
}

static size_t hexadecimal_digits(uint64_t value, int uppercase, char *digits)
{
    const char *alphabet = uppercase != 0 ?
                           "0123456789ABCDEF" : "0123456789abcdef";
    char reversed[16];
    size_t count = 0u;
    size_t index;

    do {
        reversed[count++] = alphabet[(unsigned)(value & UINT64_C(0x0f))];
        value >>= 4;
    } while (value != 0u);

    for (index = 0u; index < count; ++index)
        digits[index] = reversed[count - index - 1u];
    return count;
}

static void buffer_append_number(log_buffer_t *buffer,
                                 uint64_t magnitude,
                                 int negative,
                                 unsigned base,
                                 int uppercase,
                                 unsigned width,
                                 char padding)
{
    char digits[20];
    size_t count = base == 16u ?
                   hexadecimal_digits(magnitude, uppercase, digits) :
                   decimal_digits(magnitude, digits);
    unsigned total = (unsigned)count + (negative != 0 ? 1u : 0u);
    size_t index;

    if (negative != 0 && padding == '0')
        buffer_append_char(buffer, '-');
    if (width > total)
        buffer_append_repeat(buffer, padding, width - total);
    if (negative != 0 && padding != '0')
        buffer_append_char(buffer, '-');
    for (index = 0u; index < count; ++index)
        buffer_append_char(buffer, digits[index]);
}

static uint64_t signed_magnitude(int64_t value)
{
    if (value >= 0)
        return (uint64_t)value;
    return (uint64_t)(-(value + 1)) + UINT64_C(1);
}

static void buffer_append_format(log_buffer_t *buffer,
                                 const char *format,
                                 va_list arguments)
{
    while (*format != '\0') {
        char padding = ' ';
        unsigned width = 0u;
        int length = 0;
        char specifier;

        if (*format != '%') {
            buffer_append_char(buffer, *format++);
            continue;
        }
        ++format;
        if (*format == '%') {
            buffer_append_char(buffer, *format++);
            continue;
        }
        if (*format == '0') {
            padding = '0';
            ++format;
        }
        while (*format >= '0' && *format <= '9') {
            width = width * 10u + (unsigned)(*format - '0');
            ++format;
        }
        if (*format == 'l') {
            length = 1;
            ++format;
            if (*format == 'l') {
                length = 2;
                ++format;
            }
        } else if (*format == 'z') {
            length = 3;
            ++format;
        }
        specifier = *format;
        if (specifier == '\0') {
            buffer_append_char(buffer, '%');
            break;
        }
        ++format;

        switch (specifier) {
        case 'c':
            buffer_append_char(buffer, (char)va_arg(arguments, int));
            break;
        case 's':
            buffer_append_string(buffer, va_arg(arguments, const char *));
            break;
        case 'd':
        case 'i': {
            int64_t value;

            if (length == 2)
                value = va_arg(arguments, long long);
            else if (length == 1)
                value = va_arg(arguments, long);
            else if (length == 3)
                value = (int64_t)va_arg(arguments, ptrdiff_t);
            else
                value = va_arg(arguments, int);
            buffer_append_number(buffer,
                                 signed_magnitude(value),
                                 value < 0,
                                 10u,
                                 0,
                                 width,
                                 padding);
            break;
        }
        case 'u':
        case 'x':
        case 'X': {
            uint64_t value;

            if (length == 2)
                value = va_arg(arguments, unsigned long long);
            else if (length == 1)
                value = va_arg(arguments, unsigned long);
            else if (length == 3)
                value = va_arg(arguments, size_t);
            else
                value = va_arg(arguments, unsigned int);
            buffer_append_number(buffer,
                                 value,
                                 0,
                                 specifier == 'u' ? 10u : 16u,
                                 specifier == 'X',
                                 width,
                                 padding);
            break;
        }
        case 'p': {
            uintptr_t value = (uintptr_t)va_arg(arguments, void *);

            buffer_append_string(buffer, "0x");
            buffer_append_number(buffer,
                                 (uint64_t)value,
                                 0,
                                 16u,
                                 0,
                                 width,
                                 padding);
            break;
        }
        default:
            buffer_append_char(buffer, '%');
            buffer_append_char(buffer, specifier);
            break;
        }
    }
}

#if defined(CONFIG_ECOS_LOG_SOURCE_LOCATION) && CONFIG_ECOS_LOG_SOURCE_LOCATION
static const char *source_basename(const char *file)
{
    const char *base = file;

    if (file == (const char *)0)
        return "?";
    while (*file != '\0') {
        if (*file == '/' || *file == '\\')
            base = file + 1;
        ++file;
    }
    return base;
}
#endif

static void finish_truncated_record(log_buffer_t *buffer)
{
    static const char marker[] = "...\r\n";
    size_t marker_size = sizeof(marker) - 1u;
    size_t index;

    if (buffer->capacity <= marker_size)
        return;
    buffer->length = buffer->capacity - 1u - marker_size;
    for (index = 0u; index < marker_size; ++index)
        buffer->data[buffer->length++] = marker[index];
    buffer->data[buffer->length] = '\0';
}

ecos_err_t ecos_log_set_level(ecos_log_level_t level)
{
    if (level < ECOS_LOG_DEBUG || level > ECOS_LOG_OFF)
        return ECOS_ERR_INVALID_ARGUMENT;
    current_log_level = level;
    return ECOS_OK;
}

ecos_log_level_t ecos_log_get_level(void)
{
    return current_log_level;
}

ecos_err_t ecos_log_set_writer(ecos_log_writer_t writer, void *context)
{
    if (writer == (ecos_log_writer_t)0 && context != (void *)0)
        return ECOS_ERR_INVALID_ARGUMENT;
    log_writer = writer;
    log_writer_context = context;
    return ECOS_OK;
}

int ecos_log_vwrite(ecos_log_level_t level,
                    const char *tag,
                    const char *file,
                    int line,
                    const char *format,
                    va_list arguments)
{
    char storage[CONFIG_ECOS_LOG_BUFFER_SIZE];
    log_buffer_t buffer = { storage, sizeof(storage), 0u, 0 };
    ecos_log_writer_t writer = log_writer;
    void *writer_context = log_writer_context;
    int result;

    storage[0] = '\0';
    if (level < ECOS_LOG_DEBUG || level >= ECOS_LOG_OFF ||
        format == (const char *)0)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (level < current_log_level || current_log_level == ECOS_LOG_OFF)
        return ECOS_OK;

#if defined(CONFIG_ECOS_LOG_COLOR) && CONFIG_ECOS_LOG_COLOR
    if (level == ECOS_LOG_WARN)
        buffer_append_string(&buffer, "\033[33m");
    else if (level == ECOS_LOG_ERROR)
        buffer_append_string(&buffer, "\033[31m");
    else if (level == ECOS_LOG_FATAL)
        buffer_append_string(&buffer, "\033[35m");
#endif
    buffer_append_char(&buffer, '[');
    buffer_append_char(&buffer, level_characters[level]);
    buffer_append_string(&buffer, "][");
    buffer_append_string(&buffer, tag != (const char *)0 ? tag : "-");
    buffer_append_string(&buffer, "] ");
#if defined(CONFIG_ECOS_LOG_COLOR) && CONFIG_ECOS_LOG_COLOR
    buffer_append_string(&buffer, "\033[0m");
#endif
    buffer_append_format(&buffer, format, arguments);
#if defined(CONFIG_ECOS_LOG_SOURCE_LOCATION) && CONFIG_ECOS_LOG_SOURCE_LOCATION
    buffer_append_string(&buffer, " @ ");
    buffer_append_string(&buffer, source_basename(file));
    buffer_append_char(&buffer, ':');
    buffer_append_number(&buffer,
                         signed_magnitude((int64_t)line),
                         line < 0,
                         10u,
                         0,
                         0u,
                         ' ');
#else
    (void)file;
    (void)line;
#endif
    buffer_append_char(&buffer, '\r');
    buffer_append_char(&buffer, '\n');
    if (buffer.truncated != 0)
        finish_truncated_record(&buffer);

    if (writer == (ecos_log_writer_t)0) {
        writer = ecos_log_platform_write;
        writer_context = (void *)0;
    }
    result = writer(writer_context, buffer.data, buffer.length);
    if (result < 0)
        return result;
    if ((size_t)result != buffer.length)
        return ECOS_ERR_IO;
    return result;
}

int ecos_log_write(ecos_log_level_t level,
                   const char *tag,
                   const char *file,
                   int line,
                   const char *format,
                   ...)
{
    va_list arguments;
    int result;

    va_start(arguments, format);
    result = ecos_log_vwrite(level, tag, file, line, format, arguments);
    va_end(arguments);
    return result;
}

int ecos_log_error(const char *tag,
                   ecos_err_t error,
                   const char *operation,
                   const char *file,
                   int line)
{
    const char *description;

    if (error >= 0 || operation == (const char *)0)
        return ECOS_ERR_INVALID_ARGUMENT;
    description = ecos_err_description(error);
    if (description == (const char *)0)
        return ecos_log_write(ECOS_LOG_ERROR,
                              tag,
                              file,
                              line,
                              "%s: %s (%d)",
                              operation,
                              ecos_err_name(error),
                              (int)error);
    return ecos_log_write(ECOS_LOG_ERROR,
                          tag,
                          file,
                          line,
                          "%s: %s (%d): %s",
                          operation,
                          ecos_err_name(error),
                          (int)error,
                          description);
}

void ecos_panic_set_handler(ecos_panic_handler_t handler, void *context)
{
    panic_handler = handler;
    panic_handler_context = context;
}

void ecos_panic(void)
{
    if (panic_handler != (ecos_panic_handler_t)0)
        panic_handler(panic_handler_context);
    for (;;)
        ;
}
