#ifndef __STDLIB_H
#define __STDLIB_H

#include "sys/types.h"
#include "macros.h"
#include "ucc_attr.h"

int atoi(const char *);

void *malloc(size_t) __warn_unused;
void *calloc(size_t count, size_t) __warn_unused;
void *realloc(void *, size_t) __warn_unused;
void  free(   void *);

void exit(int);
void abort(void);

char *getenv(const char *);

extern char *__progname;

int atexit(void (*)(void));

#endif
