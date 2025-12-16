#ifndef UTIL_H
#define UTIL_H

#include "types.h"

int memcmp(u8 *src, u8 *dst, int n);
void memcpy(char *source, char *dest, int nbytes);
void memset(u8 *dest, u8 val, u32 len);
void int_to_ascii(int n, char str[]);
void reverse(char s[]);
int strlen(char s[]);

#endif
