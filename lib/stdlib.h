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
int atexit(void (*)(void));
int atexit_b(void (^)(void));

void abort(void);

char *getenv(const char *);

extern char *__progname;

/* c99 */
void _Exit(int);

/* c11 */
int at_quick_exit(void (*)(void));
void quick_exit(int);

typedef struct
{
	int quot, rem;
} div_t;

div_t div(int n, int denom);
//ldiv_t ldiv(long int n, long int denom);

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

#endif
