#include "stdio.h"
#include "string.h"

// 辅助函数：输出字符串
static int puts_helper(const char *str) {
    int count = 0;
    while (*str) {
        hal_sys_putchar(*str++);
        count++;
    }
    return count;
}

int puts(const char *str) {
    int count = puts_helper(str);
    hal_sys_putchar('\n');
    return count + 1;
}

// 辅助函数：将整数转换为字符串（十进制）
static int itoa_decimal(int value, char *buffer, int min_width, char pad_char) {
    char temp[12]; // 足够存储 32 位整数
    int i = 0;
    int is_negative = 0;
    
    if (value < 0) {
        is_negative = 1;
        value = -value;
    }
    
    if (value == 0) {
        temp[i++] = '0';
    } else {
        while (value > 0) {
            temp[i++] = '0' + (value % 10);
            value /= 10;
        }
    }
    
    int len = 0;
    if (is_negative && pad_char == '0') {
        buffer[len++] = '-';
    }

    while (i + is_negative < min_width) {
        buffer[len++] = pad_char;
        min_width--;
    }

    if (is_negative && pad_char != '0') {
        buffer[len++] = '-';
    }
    
    // 反转数字
    while (i > 0) {
        buffer[len++] = temp[--i];
    }
    buffer[len] = '\0';
    
    return len;
}

// 辅助函数：将整数转换为十六进制字符串
static int itoa_hex(unsigned long value, char *buffer, int uppercase, int min_width, char pad_char) {
    char temp[17]; // 足够存储 64 位十六进制
    int i = 0;
    const char *digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
    
    if (value == 0) {
        temp[i++] = '0';
    } else {
        while (value > 0) {
            temp[i++] = digits[value % 16];
            value /= 16;
        }
    }
    
    int len = 0;
    while (i < min_width) {
        buffer[len++] = pad_char;
        min_width--;
    }

    // 反转数字
    while (i > 0) {
        buffer[len++] = temp[--i];
    }
    buffer[len] = '\0';
    
    return len;
}

// vprintf 实现
int vprintf(const char *fmt, va_list args) {
    int count = 0;
    char buffer[32];
    
    while (*fmt) {
        if (*fmt == '%' && *(fmt + 1)) {
            fmt++; // 跳过 '%'
            
            char pad_char = ' ';
            int min_width = 0;
            
            // 解析前导零
            if (*fmt == '0') {
                pad_char = '0';
                fmt++;
            }
            
            // 解析宽度
            while (*fmt >= '0' && *fmt <= '9') {
                min_width = min_width * 10 + (*fmt - '0');
                fmt++;
            }
            
            switch (*fmt) {
                case 'c': {
                    // 字符
                    char c = (char)va_arg(args, int);
                    hal_sys_putchar(c);
                    count++;
                    break;
                }
                
                case 's': {
                    // 字符串
                    const char *str = va_arg(args, const char*);
                    if (str == NULL) {
                        str = "(null)";
                    }
                    count += puts_helper(str);
                    break;
                }
                
                case 'd':
                case 'i': {
                    // 十进制整数
                    int value = va_arg(args, int);
                    itoa_decimal(value, buffer, min_width, pad_char);
                    count += puts_helper(buffer);
                    break;
                }
                
                case 'u': {
                    // 无符号十进制整数
                    unsigned int value = va_arg(args, unsigned int);
                    itoa_decimal((int)value, buffer, min_width, pad_char); // 简化处理
                    count += puts_helper(buffer);
                    break;
                }
                
                case 'x': {
                    // 小写十六进制
                    unsigned int value = va_arg(args, unsigned int);
                    itoa_hex(value, buffer, 0, min_width, pad_char);
                    count += puts_helper(buffer);
                    break;
                }
                
                case 'X': {
                    // 大写十六进制
                    unsigned int value = va_arg(args, unsigned int);
                    itoa_hex(value, buffer, 1, min_width, pad_char);
                    count += puts_helper(buffer);
                    break;
                }
                
                case 'p': {
                    // 指针（作为十六进制处理）
                    void *ptr = va_arg(args, void*);
                    hal_sys_putchar('0');
                    hal_sys_putchar('x');
                    count += 2;
                    itoa_hex((unsigned long)ptr, buffer, 0, min_width, pad_char);
                    count += puts_helper(buffer);
                    break;
                }
                
                case '%': {
                    // 字面量 '%'
                    hal_sys_putchar('%');
                    count++;
                    break;
                }

                case 'l': {
                    // 长整数处理 (简化)
                    fmt++;
                    if (*fmt == 'l') {
                        fmt++;
                    }
                    if (*fmt == 'x' || *fmt == 'X') {
                        unsigned long value = va_arg(args, unsigned long);
                        itoa_hex(value, buffer, *fmt == 'X', min_width, pad_char);
                        count += puts_helper(buffer);
                    } else if (*fmt == 'd' || *fmt == 'i' || *fmt == 'u') {
                        long value = va_arg(args, long);
                        itoa_decimal(value, buffer, min_width, pad_char);
                        count += puts_helper(buffer);
                    } else {
                        hal_sys_putchar('%');
                        hal_sys_putchar(*fmt);
                        count += 2;
                    }
                    break;
                }
                
                default: {
                    // 不支持的格式，输出原样
                    hal_sys_putchar('%');
                    hal_sys_putchar(*fmt);
                    count += 2;
                    break;
                }
            }
        } else {
            // 普通字符
            hal_sys_putchar(*fmt);
            count++;
        }
        fmt++;
    }
    
    return count;
}



// vsnprintf 实现
int vsnprintf(char *str, size_t size, const char *fmt, va_list args) {
    if (size == 0) {
        return 0;
    }

    int count = 0;          // 应该写入的总字符数（不含结尾 '\0'）
    char *dest = str;
    char buffer[32];        // 用于临时存储整数/十六进制转换的缓冲区
    size_t remaining = size - 1;  // 留一个位置给 '\0'

    while (*fmt) {
        if (*fmt == '%' && *(fmt + 1)) {
            fmt++; // 跳过 '%'

            char pad_char = ' ';
            int min_width = 0;

            // 解析前导零
            if (*fmt == '0') {
                pad_char = '0';
                fmt++;
            }

            // 解析宽度
            while (*fmt >= '0' && *fmt <= '9') {
                min_width = min_width * 10 + (*fmt - '0');
                fmt++;
            }

            switch (*fmt) {
                case 'c': {
                    char c = (char)va_arg(args, int);
                    if (remaining > 0) {
                        *dest++ = c;
                        remaining--;
                    }
                    count++;
                    break;
                }

                case 's': {
                    const char *src = va_arg(args, const char*);
                    if (src == NULL) {
                        src = "(null)";
                    }
                    while (*src) {
                        if (remaining > 0) {
                            *dest++ = *src;
                            remaining--;
                        }
                        count++;
                        src++;
                    }
                    break;
                }

                case 'd':
                case 'i': {
                    int value = va_arg(args, int);
                    int len = itoa_decimal(value, buffer, min_width, pad_char);
                    for (int i = 0; i < len; i++) {
                        if (remaining > 0) {
                            *dest++ = buffer[i];
                            remaining--;
                        }
                        count++;
                    }
                    break;
                }

                case 'u': {
                    unsigned int value = va_arg(args, unsigned int);
                    int len = itoa_decimal((int)value, buffer, min_width, pad_char);
                    for (int i = 0; i < len; i++) {
                        if (remaining > 0) {
                            *dest++ = buffer[i];
                            remaining--;
                        }
                        count++;
                    }
                    break;
                }

                case 'x': {
                    unsigned int value = va_arg(args, unsigned int);
                    int len = itoa_hex(value, buffer, 0, min_width, pad_char);
                    for (int i = 0; i < len; i++) {
                        if (remaining > 0) {
                            *dest++ = buffer[i];
                            remaining--;
                        }
                        count++;
                    }
                    break;
                }

                case 'X': {
                    unsigned int value = va_arg(args, unsigned int);
                    int len = itoa_hex(value, buffer, 1, min_width, pad_char);
                    for (int i = 0; i < len; i++) {
                        if (remaining > 0) {
                            *dest++ = buffer[i];
                            remaining--;
                        }
                        count++;
                    }
                    break;
                }

                case 'p': {
                    void *ptr = va_arg(args, void*);
                    // 处理 "0x" 前缀
                    if (remaining > 0) {
                        *dest++ = '0';
                        remaining--;
                    }
                    count++;
                    if (remaining > 0) {
                        *dest++ = 'x';
                        remaining--;
                    }
                    count++;
                    int len = itoa_hex((unsigned long)ptr, buffer, 0, min_width, pad_char);
                    for (int i = 0; i < len; i++) {
                        if (remaining > 0) {
                            *dest++ = buffer[i];
                            remaining--;
                        }
                        count++;
                    }
                    break;
                }

                case '%': {
                    if (remaining > 0) {
                        *dest++ = '%';
                        remaining--;
                    }
                    count++;
                    break;
                }

                case 'l': {
                    fmt++;
                    if (*fmt == 'l') {
                        fmt++;
                    }
                    if (*fmt == 'x' || *fmt == 'X') {
                        unsigned long value = va_arg(args, unsigned long);
                        int len = itoa_hex(value, buffer, (*fmt == 'X'), min_width, pad_char);
                        for (int i = 0; i < len; i++) {
                            if (remaining > 0) {
                                *dest++ = buffer[i];
                                remaining--;
                            }
                            count++;
                        }
                    } else if (*fmt == 'd' || *fmt == 'i' || *fmt == 'u') {
                        long value = va_arg(args, long);
                        int len = itoa_decimal(value, buffer, min_width, pad_char);
                        for (int i = 0; i < len; i++) {
                            if (remaining > 0) {
                                *dest++ = buffer[i];
                                remaining--;
                            }
                            count++;
                        }
                    } else {
                        if (remaining > 0) {
                            *dest++ = '%';
                            remaining--;
                        }
                        count++;
                        if (remaining > 0) {
                            *dest++ = *fmt;
                            remaining--;
                        }
                        count++;
                    }
                    break;
                }

                default: {
                    if (remaining > 0) {
                        *dest++ = '%';
                        remaining--;
                    }
                    count++;
                    if (remaining > 0) {
                        *dest++ = *fmt;
                        remaining--;
                    }
                    count++;
                    break;
                }
            }
        } else {
            if (remaining > 0) {
                *dest++ = *fmt;
                remaining--;
            }
            count++;
        }
        fmt++;
    }

    *dest = '\0';   // 添加字符串终止符
    return count;
}

int snprintf(char *buffer, size_t size, const char *format, ...){
    va_list args;
    int result;
    
    va_start(args, format);
    result = vsnprintf(buffer, size, format, args);
    va_end(args);
    
    return result;
}

// printf 实现
int printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int ret = vprintf(fmt, args);
    va_end(args);
    return ret;
}
