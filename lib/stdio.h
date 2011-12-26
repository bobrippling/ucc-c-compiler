#ifndef __STDIO_H
#define __STDIO_H

typedef int FILE;
typedef int size_t;

extern FILE *stdin, *stdout, *stderr;

/* va_list */
#include "stdarg.h"

int puts( const char *);
int printf(const char *, ...);
int fprintf(FILE *, const char *, ...);

int dprintf(int, const char *, ...);

#endif
