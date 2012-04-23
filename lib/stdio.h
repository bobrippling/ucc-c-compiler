#ifndef __STDIO_H
#define __STDIO_H

#define __STDIO_FILE_SIMPLE

#ifdef __STDIO_FILE_SIMPLE
typedef int FILE;
#else
typedef struct __FILE
{
	int fd;
	enum { __FILE_fine, __FILE_eof, __FILE_err } status;
} FILE;
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

/* TODO: func interface */

typedef size_t fpos_t;
FILE	*funopen(
		const void *,
		int    (*)(void *, char *, int),
		int    (*)(void *, const char *, int),
		fpos_t (*)(void *, fpos_t, int),
		int    (*)(void *)
	);


/* status */
int   feof(FILE *);
int   ferror(FILE *);

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

int fileno(FILE *);
#ifdef __STDIO_FILE_SIMPLE
#  define fileno(f) (*(f))
#else
#  define fileno(f) ((f)->fd)
#endif

#endif
