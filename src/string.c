#include <stdint.h>

void *memset(void *s, int c, uint32_t n) {
  unsigned char *p = s;
  while(n--){
    *p = (unsigned char)c;
    p++;
  }
  return s;
}

void *memcpy(void *out, const void *in, uint32_t n) {
  char *d = out;
    const char *s = in;
    while (n--) {
        *d++ = *s++;
    }
    return out;
}
