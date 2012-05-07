#ifndef __STDIO_H
#define __STDIO_H

typedef struct __FILE
{
	int fd;
	enum { __FILE_fine, __FILE_eof, __FILE_err } status;
} FILE;

extern FILE *stdin, *stdout, *stderr;

#define EOF -1

/* va_list */
#include "stdarg.h"
/* null */
#include "macros.h"

/* io */
FILE *fopen(const char *path, const char *mode);
int   fclose(FILE *);
int   fflush(FILE *f);
FILE *freopen(const char *path, const char *mode, FILE *stream);

size_t fread(       void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);


typedef size_t fpos_t;

/* TODO: func interface */
FILE *funopen(
		const void *cookie,
		int      (*readfn)(void *,       char *, int),
		int     (*writefn)(void *, const char *, int),
		fpos_t   (*seekfn)(void *,       fpos_t, int),
		int     (*closefn)(void *)
		);

FILE *fropen(void *cookie, int (*readfn )(void *,       char *, int));
FILE *fwopen(void *cookie, int (*writefn)(void *, const char *, int));


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
#  define fileno(f) ((f)->fd)

#endif
