#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <limits.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)


int printf(const char *fmt, ...) {
  assert(strlen(fmt) < 100);
  char out[150] = {};
  // 获取可变参数
  va_list args;
  va_start(args, fmt);
  int ret = vsprintf(out, fmt, args);
  va_end(args);
  putstr(out);
  return ret;
}



int vsprintf(char *out, const char *fmt, va_list ap) {
  char *p = out;

  while (*fmt) {
    if (*fmt != '%') {
      *p++ = *fmt++;
      continue;
    }

    fmt++;  // skip '%'

    // ----------- 解析补零和宽度 -----------
    int zero_pad = 0, width = 0;
    if (*fmt == '0') {
      zero_pad = 1;
      fmt++;
    }
    while (*fmt >= '0' && *fmt <= '9') {
      width = width * 10 + (*fmt++ - '0');
    }

    // ----------- 格式处理 -----------
    switch (*fmt) {
      case 'd': {
        int num = va_arg(ap, int);
        char buf[20], *b = buf + sizeof(buf) - 1;
        *b = '\0';
        int neg = 0;

        if (num < 0) {
          neg = 1;
          if (num == INT_MIN) {
            // 特殊处理最小负数
            strcpy(buf, "-2147483648");
            b = buf;
            break;
          }
          num = -num;
        }

        do {
          *--b = '0' + (num % 10);
          num /= 10;
        } while (num);

        if (neg) *--b = '-';

        int len = strlen(b);
        while (len < width) {
          *p++ = (zero_pad && !neg) ? '0' : ' ';
          width--;
        }
        while (*b) *p++ = *b++;
        break;
      }

      case 'x': {
        unsigned int num = va_arg(ap, unsigned int);
        char buf[20], *b = buf + sizeof(buf) - 1;
        *b = '\0';
        const char *hex = "0123456789abcdef";

        do {
          *--b = hex[num % 16];
          num /= 16;
        } while (num);

        int len = strlen(b);
        while (len < width) {
          *p++ = zero_pad ? '0' : ' ';
          width--;
        }
        while (*b) *p++ = *b++;
        break;
      }

      case 's': {
        char *s = va_arg(ap, char *);
        int len = strlen(s);
        while (len < width) {
          *p++ = zero_pad ? '0' : ' ';
          width--;
        }
        while (*s) *p++ = *s++;
        break;
      }

      case 'c': {
        char c = (char)va_arg(ap, int);
        *p++ = c;
        break;
      }

      case '%': {
        *p++ = '%';
        break;
      }

      default: {
        *p++ = '%';
        *p++ = *fmt;
        break;
      }
    }

    fmt++;
  }

  *p = '\0';
  return p - out;
}


int sprintf(char *out, const char *fmt, ...) 
{
  // 获取可变参数
  va_list args;
  va_start(args, fmt);

  int ret = vsprintf(out, fmt, args);
 
  va_end(args);
  
  // 返回总长
  return ret; 
}


int snprintf(char *out, size_t n, const char *fmt, ...) {
  panic("Not implemented");
}


int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("Not implemented");
}

#endif
