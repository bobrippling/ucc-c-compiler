#ifndef __STDIO_H
#define __STDIO_H

#ifdef __TYPEDEFS_WORKING
typedef int FILE;
#else
#define FILE int
#endif

extern FILE *stdin, *stdout, *stderr;

#define EOF -1

/* va_list */
#include "stdarg.h"
/* null */
#include "macros.h"

/* io */
FILE *fopen(const char *path, const char *mode);
int   fclose(FILE *);
/* TODO: freopen */

/* writing */
int fprintf(FILE *, const char *, ...);
int  printf(const char *, ...);

int dprintf(int, const char *, ...);

int fputs(const char *, FILE *);
int  puts(const char *);

int fputc(int, FILE *);
#define putc fputc
int  putchar(int);

/* error */
void perror(const char *);

/* writing, variadic */
int vfprintf(FILE *, char *, va_list);


/* reading */
int   getchar(void);
#define getc fgetc
int   fgetc(FILE *);
/*char *gets( char *);*/
char *fgets(char *, int, FILE *);


/* file system */
int remove(const char *);

#endif
