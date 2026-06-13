#ifndef __CTYPE_H
#define __CTYPE_H

#ifdef __cplusplus
extern "C" {
#endif

int iscntrl(int c);

int isblank(int c);

int isspace(int c);

int isupper(int c);

int islower(int c);

int isalpha(int c);

int isdigit(int c);

int isxdigit(int c);

int isalnum(int c);

int ispunct(int c);

int isgraph(int c);

int isprint(int c);

int tolower(int c);

int toupper(int c);

#ifdef __cplusplus
}
#endif

#endif /* __CTYPE_H */

