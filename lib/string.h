#ifndef __STRING_H
#define __STRING_H

#include "macros.h"
#include "sys/types.h"

size_t strlen(const char *);
const char *strerror(int eno);

int strcmp( const char *, const char *);
int strncmp(const char *, const char *, size_t);

char *strchr(const char *s, char c);

void *memset(void *, unsigned char c, size_t len);
void *memcpy(void *dest, const void *src, size_t n);
int memcmp(const void *, const void *, size_t n);

char *strcpy( char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t n);

#endif
