#ifndef __STDIO_H
#define __STDIO_H

typedef struct __FILE FILE;

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
FILE *fdopen(int fd, const char *mode);

size_t fread(       void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);


typedef size_t fpos_t;

typedef size_t   __stdio_read(void *,       char *, int);
typedef size_t  __stdio_write(void *, const char *, int);
typedef int      __stdio_seek(void *,       fpos_t, int);
typedef int     __stdio_close(void *);

/* func interface */
FILE *funopen(void *cookie,
		__stdio_read *, __stdio_write *, __stdio_seek *, __stdio_close *);
FILE *fropen(void *cookie, __stdio_read  *);
FILE *fwopen(void *cookie, __stdio_write *);

/* seeking */
#ifndef SEEK_SET
# define SEEK_SET  0
# define SEEK_CUR  1
# define SEEK_END  2
#endif
int fseek(FILE *stream, long offset, int whence);

long ftell(FILE *stream);

void rewind(FILE *stream);

int fgetpos(FILE *stream, fpos_t *pos);
int fsetpos(FILE *stream, fpos_t *pos);

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
int vprintf(const char *, va_list);
int vfprintf(FILE *, const char *, va_list);


/* reading */
int   getchar(void);
#define getc fgetc
int   fgetc(FILE *);
/*char *gets( char *);*/
char *fgets(char *, int, FILE *);

/* bsd extension */
char *fgetln(FILE *stream, size_t *len); /* valid until next read */


/* file system */
int remove(const char *);

int fileno(FILE *);

#endif
