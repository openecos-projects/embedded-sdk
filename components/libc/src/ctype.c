#include "ctype.h"

int iscntrl(int c) {
    return (c >= '\0' && c <= '\x1f') || (c == '\x7f');
}

int isblank(int c) {
    return (c == '\t' || c == ' ');
}

int isspace(int c) {
    return (c == ' ' || (c >= '\t' && c <= '\r'));
}

int isupper(int c) {
    return (c >= 'A' && c <= 'Z');
}

int islower(int c) {
    return (c >= 'a' && c <= 'z');
}

int isalpha(int c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

int isdigit(int c) {
    return (c >= '0' && c <= '9');
}

int isxdigit(int c) {
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

int isalnum(int c) {
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

int ispunct(int c) {
    return (c >= '!' && c <= '/') || 
           (c >= ':' && c <= '@') || 
           (c >= '[' && c <= '`') || 
           (c >= '{' && c <= '~');
}

int isgraph(int c) {
    return (c >= '!' && c <= '~');
}

int isprint(int c) {
    return (c >= ' ' && c <= '~');
}

int tolower(int c) {
    if (isupper(c)) {
        return c - 'A' + 'a';
    } else {
        return c;
    }
}

int toupper(int c) {
    if (islower(c)) {
        return c - 'a' + 'A';
    } else {
        return c;
    }
}
