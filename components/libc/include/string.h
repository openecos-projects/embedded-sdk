#ifndef STRING_H
#define STRING_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Copy memory from source to destination
 * @param dest Destination memory address
 * @param src Source memory address
 * @param n Number of bytes to copy
 * @return Pointer to destination
 */
void* memcpy(void* dest, const void* src, size_t n);

/**
 * @brief Set memory to a specific value
 * @param s Memory address to set
 * @param c Value to set (as unsigned char)
 * @param n Number of bytes to set
 * @return Pointer to memory address
 */
void* memset(void* s, int c, size_t n);

/**
 * @brief Calculate length of string
 * @param s String to measure
 * @return Length of string (excluding null terminator)
 */
size_t strlen(const char* s);

int memcmp(const void* ptr1, const void* ptr2, size_t num);

char* strchr(const char* str, int c);

int strcmp ( const char* src, const char* dst );

char * strcpy(char *dst,const char *src);

char *strncpy(char *dst, const char *src, size_t len);

int strncmp(const char *s1, const char *s2, size_t n);

void * memmove(void *dest, const void *src, size_t num);

#ifdef __cplusplus
}
#endif

#endif /* STRING_H */