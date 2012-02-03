#ifndef __STRING_H
#define __STRING_H

#include "macros.h"
#include "sys/types.h"

int strlen(const char *);
const char *strerror(int eno);

int strcmp( char *, char *);
int strncmp(char *, char *, size_t);

char *strchr(char *s, char c);

#endif
