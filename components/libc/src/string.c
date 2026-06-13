#include <stddef.h>
#include <stdint.h>

// 简单的 memcpy 实现
void *memcpy(void *dest, const void *src, size_t n) {
    char *d = (char *)dest;
    const char *s = (const char *)src;
    
    while (n--) {
        *d++ = *s++;
    }
    
    return dest;
}

// 简单的 memset 实现
void *memset(void *s, int c, size_t n) {
    unsigned char *p = (unsigned char *)s;
    
    while (n--) {
        *p++ = (unsigned char)c;
    }
    
    return s;
}

// 简单的 strlen 实现
size_t strlen(const char *s) {
    size_t len = 0;
    while (*s++) {
        len++;
    }
    return len;
}

int memcmp(const void* ptr1, const void* ptr2, size_t num) {
    const unsigned char *p1 = (const unsigned char*)ptr1;
    const unsigned char *p2 = (const unsigned char*)ptr2;
    for (size_t i=0; i<num; ++i) {
        if (p1[i] > p2[i]) {
            return 1;
        } else if (p1[i] < p2[i]) {
            return -1;
        }
    }
    return 0;
}

char* strchr(const char* str, int c) {
    // 标准 strchr 将 c 转换为 char
    char target = (char)c;
    
    // 循环，包括最后的 '\0'
    do {
        if (*str == target) {
            return (char*)str;  // 返回匹配位置的指针
        }
    } while (*str++ != '\0');
    
    return NULL;
}

int strcmp(const char* src, const char* dst)
{
    while (*src && *dst && *src == *dst) {
        src++;
        dst++;
    }
    return (unsigned char)*src - (unsigned char)*dst;
}

char *strncpy(char *dst, const char *src, size_t len)  
{  
    char *res = dst;  
    if (dst >= src && dst <= src + len - 1)//重叠,从后向前复制  
    {  
        dst = dst + len - 1;  
        src = src + len - 1;  
        while (len--)  
            *dst-- = *src--;  
    }  
    else  
    {  
        while (len--)  
            *dst++ = *src++;  
    }  
    return res;  
}

char *strcpy(char *dest, const char *src)
{
    char *original_dest = dest;
    
    // 逐个复制字符，直到遇到源字符串的结束符 '\0'
    while ((*dest++ = *src++) != '\0');
    
    return original_dest;
}

int strncmp(const char *s1, const char *s2, size_t n)
{
    const unsigned char *p1 = (const unsigned char *)s1;
    const unsigned char *p2 = (const unsigned char *)s2;
    size_t i;
    
    for (i = 0; i < n; i++) {
        if (p1[i] != p2[i] || p1[i] == '\0') {
            return p1[i] - p2[i];
        }
    }
    
    return 0;
}

void *memmove(void *dest, const void *src, size_t n)
{
    unsigned char *d = dest;
    const unsigned char *s = src;
    
    if (d == s || n == 0) {
        return dest;
    }
    
    // 检测内存重叠
    if (d > s && d < s + n) {
        // 重叠且目标在源之后，从后向前复制
        d += n;
        s += n;
        while (n--) {
            *--d = *--s;
        }
    } else {
        // 无重叠或目标在源之前，从前向后复制
        while (n--) {
            *d++ = *s++;
        }
    }
    
    return dest;
}
