#include "ctype.h"
#include "stdlib.h"

static unsigned int _random_seed = 0x7a2d5eed;

int rand(void) {
    unsigned int feedback = 0;
    feedback ^= (_random_seed>>31) | 1;
    feedback ^= (_random_seed>>21) | 1;
    feedback ^= (_random_seed>>1) | 1;
    feedback ^= (_random_seed>>0) | 1;
    _random_seed >>= 1;
    _random_seed |= feedback<<31;
    return (_random_seed >> 1) % RAND_MAX;
}

void srand(unsigned int seed) {
    _random_seed = seed;
}

int abs(int x) {
    return (x < 0 ? -x : x);
}

int atoi(const char* nptr) {
    int x = 0;
    int negative = 0;
    while (isspace(*nptr)) {
        nptr++;
    }
    if (*nptr == '+') {
        negative = 0;
        ++nptr;
    } else if (*nptr == '-') {
        negative = 1;
        ++nptr;
    }
    while (*nptr >= '0' && *nptr <= '9') {
        x = x * 10 + *nptr - '0';
        nptr ++;
    }
    if (negative) {
        return -x;
    } else {
        return x;
    }
}

long int atol(const char* nptr) {
    long int x = 0;
    int negative = 0;
    while (isspace(*nptr)) {
        nptr++;
    }
    if (*nptr == '+') {
        negative = 0;
        ++nptr;
    } else if (*nptr == '-') {
        negative = 1;
        ++nptr;
    }
    while (*nptr >= '0' && *nptr <= '9') {
        x = x * 10 + *nptr - '0';
        nptr ++;
    }
    if (negative) {
        return -x;
    } else {
        return x;
    }
}

long long int atoll(const char* nptr) {
    long long int x = 0;
    int negative = 0;
    while (isspace(*nptr)) {
        nptr++;
    }
    if (*nptr == '+') {
        negative = 0;
        ++nptr;
    } else if (*nptr == '-') {
        negative = 1;
        ++nptr;
    }
    while (*nptr >= '0' && *nptr <= '9') {
        x = x * 10 + *nptr - '0';
        nptr ++;
    }
    if (negative) {
        return -x;
    } else {
        return x;
    }
}

long int strtol(const char* str, char** endptr, int base) {
    size_t i = 0;
    int negative = 0;
    long int result = 0;
    // fail if `base` is invalid
    if (base!=0 && (base<2 || base>36)) {
        goto finish;
    }
    // skip all whitespaces
    while (isspace(str[i])) {
        ++i;
    }
    // detect sign
    if (str[i] == '+') {
        negative = 0;
        ++i;
    } else if (str[i] == '-') {
        negative = 1;
        ++i;
    }
    // detect base
    if (base == 0) {        
        if (str[i] == '0') {
            ++i;
            if (str[i]=='x' || str[i]=='X') {
                base = 16;
                ++i;
            } else {
                base = 8;
            }
        } else {
            base = 10;
        }
    } else if (base == 16) {
        // sikp the "0x" prefix for hexadecimal
        if (str[i] == '0') {
            ++i;
        }
        if (str[i]=='x' || str[i]=='X') {
            ++i;
        }
    }
    // parse number
    while (1) {
        long int digit;
        if (isdigit(str[i])) {
            digit = str[i] - '0';
        } else if (isupper(str[i])) {
            digit = str[i] - 'A' + 10;
        } else if (islower(str[i])) {
            digit = str[i] - 'a' + 10;
        } else {
            break;
        }
        if (digit >= base) {
            break;
        }
        result *= base;
        result += digit;
        ++i;
    }
    finish:
    if (endptr != NULL) {
        *endptr = (char*)str + i;
    }
    if (negative) {
        return -result;
    } else {
        return result;
    }
}

long long int strtoll(const char* str, char** endptr, int base) {
    size_t i = 0;
    int negative = 0;
    long long int result = 0;
    // fail if `base` is invalid
    if (base!=0 && (base<2 || base>36)) {
        goto finish;
    }
    // skip all whitespaces
    while (isspace(str[i])) {
        ++i;
    }
    // detect sign
    if (str[i] == '+') {
        negative = 0;
        ++i;
    } else if (str[i] == '-') {
        negative = 1;
        ++i;
    }
    // detect base
    if (base == 0) {        
        if (str[i] == '0') {
            ++i;
            if (str[i]=='x' || str[i]=='X') {
                base = 16;
                ++i;
            } else {
                base = 8;
            }
        } else {
            base = 10;
        }
    } else if (base == 16) {
        // sikp the "0x" prefix for hexadecimal
        if (str[i] == '0') {
            ++i;
        }
        if (str[i]=='x' || str[i]=='X') {
            ++i;
        }
    }
    // parse number
    while (1) {
        long int digit;
        if (isdigit(str[i])) {
            digit = str[i] - '0';
        } else if (isupper(str[i])) {
            digit = str[i] - 'A' + 10;
        } else if (islower(str[i])) {
            digit = str[i] - 'a' + 10;
        } else {
            break;
        }
        if (digit >= base) {
            break;
        }
        result *= base;
        result += digit;
        ++i;
    }
    finish:
    if (endptr != NULL) {
        *endptr = (char*)str + i;
    }
    if (negative) {
        return -result;
    } else {
        return result;
    }
}

unsigned long int strtoul(const char* str, char** endptr, int base) {
    size_t i = 0;
    unsigned long int result = 0;
    // fail if `base` is invalid
    if (base!=0 && (base<2 || base>36)) {
        goto finish;
    }
    // skip all whitespaces
    while (isspace(str[i])) {
        ++i;
    }
    // detect base
    if (base == 0) {        
        if (str[i] == '0') {
            ++i;
            if (str[i]=='x' || str[i]=='X') {
                base = 16;
                ++i;
            } else {
                base = 8;
            }
        } else {
            base = 10;
        }
    } else if (base == 16) {
        // sikp the "0x" prefix for hexadecimal
        if (str[i] == '0') {
            ++i;
        }
        if (str[i]=='x' || str[i]=='X') {
            ++i;
        }
    }
    // parse number
    while (1) {
        long int digit;
        if (isdigit(str[i])) {
            digit = str[i] - '0';
        } else if (isupper(str[i])) {
            digit = str[i] - 'A' + 10;
        } else if (islower(str[i])) {
            digit = str[i] - 'a' + 10;
        } else {
            break;
        }
        if (digit >= base) {
            break;
        }
        result *= base;
        result += digit;
        ++i;
    }
    finish:
    if (endptr != NULL) {
        *endptr = (char*)str + i;
    }
    return result;
}

unsigned long long int strtoull(const char* str, char** endptr, int base) {
    size_t i = 0;
    unsigned long long int result = 0;
    // fail if `base` is invalid
    if (base!=0 && (base<2 || base>36)) {
        goto finish;
    }
    // skip all whitespaces
    while (isspace(str[i])) {
        ++i;
    }
    // detect base
    if (base == 0) {        
        if (str[i] == '0') {
            ++i;
            if (str[i]=='x' || str[i]=='X') {
                base = 16;
                ++i;
            } else {
                base = 8;
            }
        } else {
            base = 10;
        }
    } else if (base == 16) {
        // sikp the "0x" prefix for hexadecimal
        if (str[i] == '0') {
            ++i;
        }
        if (str[i]=='x' || str[i]=='X') {
            ++i;
        }
    }
    // parse number
    while (1) {
        long int digit;
        if (isdigit(str[i])) {
            digit = str[i] - '0';
        } else if (isupper(str[i])) {
            digit = str[i] - 'A' + 10;
        } else if (islower(str[i])) {
            digit = str[i] - 'a' + 10;
        } else {
            break;
        }
        if (digit >= base) {
            break;
        }
        result *= base;
        result += digit;
        ++i;
    }
    finish:
    if (endptr != NULL) {
        *endptr = (char*)str + i;
    }
    return result;
}

void *malloc(size_t size) {
    return NULL;
}

void free(void *ptr) {
    return;
}

__attribute__ ((__noreturn__))
void exit(int code) {
    while (1);
}

__attribute__ ((__noreturn__))
void abort(void) {
    while (1);
}
