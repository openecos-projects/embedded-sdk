#include <am.h>
#include <klib.h>
#include <klib-macros.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)
static unsigned long int next = 1;

int rand(void) {
  // RAND_MAX assumed to be 32767
  next = next * 1103515245 + 12345;
  return (unsigned int)(next/65536) % 32768;
}

void srand(unsigned int seed) {
  next = seed;
}

int abs(int x) {
  return (x < 0 ? -x : x);
}

int atoi(const char* nptr) {
  int x = 0;
  while (*nptr == ' ') { nptr ++; }
  while (*nptr >= '0' && *nptr <= '9') {
    x = x * 10 + *nptr - '0';
    nptr ++;
  }
  return x;
}

char *itoa(unsigned int val, char *str, int base) {
  char *p = str;
  while(val) {
    int tmp = val % base;
    if(tmp <= 9)
      *p++ = '0' + tmp;
    else
      *p++ = 'A' + tmp - 10;
    val /= base;
  }
  for(char *i = str, *j = p - 1; i < p; ++i, --j) {
    if(i >= j) break;
    char tmp = *i;
    *i = *j;
    *j = tmp;
  }
  *p = 0;
  return str;
}

void *malloc(size_t size) {
  static void *program_break = 0;
  if (program_break == 0) {
    if (heap.start == 0) return 0;
    program_break = heap.start;
    //#endif
  }
  size = (size + 15) & ~15;
  void *mem = program_break;
  program_break += size;
  //assert(program_break <= heap.end, "Run out of memory");
  return mem;
}

void free(void *ptr) {
}

#endif
