#ifndef __STDLIB_H
#define __STDLIB_H

#include <stddef.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void* start;
    void* end;
} area_t;

extern const area_t heap;

__attribute__ ((__noreturn__))
void halt(int code);

#ifndef NULL
#define NULL 0
#endif

#define RAND_MAX 0x7fffffff;

void srand(unsigned int seed);

int rand(void);

void *malloc(size_t size);

void free(void *ptr);

int abs(int x);

int atoi(const char *nptr);

__attribute__ ((__noreturn__))
void exit(int code);

__attribute__ ((__noreturn__))
void abort(void);

#ifdef __cplusplus
}
#endif

#endif /* __STDLIB_H */
