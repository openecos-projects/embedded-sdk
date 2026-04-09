#include "stdio.h"
#include "string.h"

// 辅助函数：输出字符串
static int puts_helper(const char *str) {
    int count = 0;
    while (*str) {
        sys_putchar(*str++);
        count++;
    }
    return count;
}

// 辅助函数：将整数转换为字符串（十进制）
static int itoa_decimal(int value, char *buffer) {
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
    if (is_negative) {
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
static int itoa_hex(unsigned long value, char *buffer, int uppercase) {
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
            
            switch (*fmt) {
                case 'c': {
                    // 字符
                    char c = (char)va_arg(args, int);
                    sys_putchar(c);
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
                    int len = itoa_decimal(value, buffer);
                    count += puts_helper(buffer);
                    break;
                }
                
                case 'u': {
                    // 无符号十进制整数
                    unsigned int value = va_arg(args, unsigned int);
                    int len = itoa_decimal((int)value, buffer); // 简化处理
                    count += puts_helper(buffer);
                    break;
                }
                
                case 'x': {
                    // 小写十六进制
                    unsigned int value = va_arg(args, unsigned int);
                    int len = itoa_hex(value, buffer, 0);
                    count += puts_helper(buffer);
                    break;
                }
                
                case 'X': {
                    // 大写十六进制
                    unsigned int value = va_arg(args, unsigned int);
                    int len = itoa_hex(value, buffer, 1);
                    count += puts_helper(buffer);
                    break;
                }
                
                case 'p': {
                    // 指针（作为十六进制处理）
                    void *ptr = va_arg(args, void*);
                    sys_putchar('0');
                    sys_putchar('x');
                    count += 2;
                    int len = itoa_hex((unsigned long)ptr, buffer, 0);
                    count += puts_helper(buffer);
                    break;
                }
                
                case '%': {
                    // 字面量 '%'
                    sys_putchar('%');
                    count++;
                    break;
                }

                case 'l': {
                    // 长整数
                    fmt++;
                    switch (*fmt) {
                        case 'l': {
                            // 长整数
                            fmt++;
                            switch (*fmt) {
                                case 'x': {
                                    // 长整数
                                    unsigned long value = va_arg(args, unsigned long);
                                    int len = itoa_hex(value, buffer, 0);
                                    count += puts_helper(buffer);
                                    break;
                                    }
                                }
                            }
                        }
                    }
                
                default: {
                    // 不支持的格式，输出原样
                    sys_putchar('%');
                    sys_putchar(*fmt);
                    count += 2;
                    break;
                }
            }
        } else {
            // 普通字符
            sys_putchar(*fmt);
            count++;
        }
        fmt++;
    }
    
    return count;
}

// printf 实现
int printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int ret = vprintf(fmt, args);
    va_end(args);
    return ret;
}