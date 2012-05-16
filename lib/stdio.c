#include "unistd.h"
#include "stdio.h"
#include "string.h"
#include "errno.h"
#include "stdlib.h"
#include "sys/fcntl.h"
#include "assert.h"
#include "ctype.h"

#include "ucc_attr.h"

#define PRINTF_OPTIMISE

static FILE _stdin  = { 0, 0 };
static FILE _stdout = { 1, 0 };
static FILE _stderr = { 2, 0 };

FILE *stdin  = &_stdin;
FILE *stdout = &_stdout;
FILE *stderr = &_stderr;

static const char *nums = "0123456789abcdef";

/* Private */
static void printd_rec(int fd, int n, int base)
{
	int d;
	d = n / base;
	if(d)
		printd_rec(fd, d, base);
	write(fd, nums + n % base, 1);
}

static void printn(int fd, int n, int base, int is_signed)
{
	if(is_signed && n < 0){
		write(fd, "-", 1);
		n = -n;
	}

	printd_rec(fd, n, base);
}

static void printd(int fd, int n, int is_signed)
{
	printn(fd, n, 10, is_signed);
}

static void printx(int fd, int n, int is_signed)
{
	printn(fd, n, 16, is_signed);
}

static void printo(int fd, int n, int is_signed)
{
	printn(fd, n, 8, is_signed);
}

/* Public */
int feof(FILE *f __unused)
{
	return f->status == __FILE_eof;
}

int ferror(FILE *f __unused)
{
	return f->status == __FILE_err;
}

static int fopen2(FILE *f, const char *path, const char *smode)
{
	int fd, mode;
	int got_primary;

#define PRIMARY_CHECK() \
	if(got_primary) \
		goto inval; \
	got_primary = 1

	got_primary = mode = 0;

	if(!*smode)
		goto inval;

	while(*smode)
		switch(*smode++){
			case 'b':
				break;
			case 'r':
				PRIMARY_CHECK();
				if(mode & O_CREAT)
					goto inval;
				mode |= O_RDONLY;
				break;
			case 'w':
				PRIMARY_CHECK();
				mode |= O_WRONLY | O_CREAT | O_TRUNC;
				break;
			case 'a':
				PRIMARY_CHECK();
				mode |= O_WRONLY | O_CREAT; /* ? */
				break;
			case '+':
				mode &= ~(O_WRONLY | O_RDONLY);
				mode |= O_RDWR;
				break;
			default:
inval:
				errno = EINVAL;
				return 1;
		}

#undef PRIMARY_CHECK

	fd = open(path, mode, 0644);
	if(fd == -1)
		return 1;

	fileno(f) = fd;

	f->status = __FILE_fine;

	return 0;
}

static int fclose2(FILE *f)
{
	if(fflush(f))
		return EOF;
	return close(fileno(f)) == 0 ? 0 : EOF;
}

FILE *fopen(const char *path, const char *mode)
{
	FILE *f = malloc(sizeof *f);
	if(!f)
		return NULL;

	if(fopen2(f, path, mode)){
		free(f);
		return NULL;
	}
	return f;
}

FILE *freopen(const char *path, const char *mode, FILE *f)
{
	if(fclose2(f))
		return NULL;

	if(fopen2(f, path, mode)){
		free(f);
		return NULL;
	}
	return f;
}

int fclose(FILE *f)
{
	int r = fclose2(f);
	free(f);
	return r;
}

int fflush(FILE *f)
{
	return 0;
}

int fputc(int c, FILE *f)
{
	return write(fileno(f), &c, 1) == 1 ? c : EOF;
}

int putchar(int c)
{
	return fputc(c, stdout);
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	int n;
	n = read(fileno(stream), ptr, size * nmemb);
	if(n == 0){
		stream->status = __FILE_eof;
	}else if(n < 0){
		stream->status = __FILE_err;
	}else{
		return n;
	}
	return 0;
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	int n;
	n = write(fileno(stream), ptr, size * nmemb);
	return n > 0 ? n : 0;
}

int vfprintf(FILE *file, const char *fmt, va_list ap)
{
	int fd = fileno(file);
#ifdef PRINTF_OPTIMISE
	const char *buf  = fmt;
	int buflen = 0;
#else
# warning printf unoptimised
#endif

	if(!fmt)
		return;

	while(*fmt){
		if(*fmt == '%'){
			int pad = 0;

#ifdef PRINTF_OPTIMISE
			if(buflen)
				write(fd, buf, buflen); // TODO: errors
#endif

			fmt++;

			if(*fmt == '0'){
				// %0([0-9]+)d
				fmt++;
				pad = 0;
				while(isdigit(*fmt)){
					pad += *fmt - '0';
					fmt++;
				}
			}

			switch(*fmt){
				case 's':
				{
					const char *s = va_arg(ap, const char *);
					if(!s)
						s = "(null)";
					write(fd, s, strlen(s));
					break;
				}
				case 'c':
					fputc(va_arg(ap, char), file);
					break;
				case 'u':
				case 'd':
				{
					const int n = va_arg(ap, int);

					if(pad){
						if(n){
							int len = 0, copy = n;

							while(copy){
								copy /= 10;
								len++;
							}

							while(pad-- > len)
								putchar('0');
						}else{
							while((--pad))
								putchar('0');
						}
					}

					printd(fd, n, *fmt == 'd');
					break;
				}
				case 'p':
				{
					void *p = va_arg(ap, void *);
					if(p){
						/* TODO - intptr_t */
						typedef int long;
						fputs("0x", file);
						printx(fd, (long)p, 0);
					}else{
						fputs("(nil)", file);
					}
					break;
				}
				case 'x':
					printx(fd, va_arg(ap, int), 1);
					break;
				case 'o':
					printo(fd, va_arg(ap, int), 1);
					break;

				default:
					write(fd, fmt, 1); /* default to just printing the char */
			}

#ifdef PRINTF_OPTIMISE
			buf = fmt + 1;
			buflen = 0;
#endif
		}else{
#ifdef PRINTF_OPTIMISE
			buflen++;
#else
			write(fd, fmt, 1);
#endif
		}
		fmt++;
	}

#ifdef PRINTF_OPTIMISE
	if(buflen)
		write(fd, buf, buflen);
#endif
}

int dprintf(int fd, const char *fmt, ...)
{
	va_list l;
	int r;
	FILE f;

	f.fd = fd;
	f.status = __FILE_fine;

	va_start(l, fmt);
	r = vfprintf(&f, fmt, l);
	va_end(l);

	return r;
}

int fprintf(FILE *file, const char *fmt, ...)
{
	va_list l;
	int r;

	va_start(l, fmt);
	r = vfprintf(file, fmt, l);
	va_end(l);

	return r;
}

int printf(const char *fmt, ...)
{
	va_list l;
	int r;

	va_start(l, fmt);
	r = vfprintf(stdout, fmt, l);
	va_end(l);

	return r;
}

int fputs(const char *s, FILE *f)
{
	return write(fileno(f), s, strlen(s)) > 0 ? 1 : EOF;
}

int puts(const char *s)
{
	return printf("%s\n", s) > 0 ? 1 : EOF;
}

int getchar()
{
	return fgetc(stdin);
}

int fgetc(FILE *f)
{
	int ch;
	return read(fileno(f), &ch, 1) == 1 ? ch : EOF;
}

char *fgets(char *s, int l, FILE *f)
{
	int r;

	r = read(fileno(f), s, l - 1);

	switch(r){
		case  0: /* EOF */
			f->status = __FILE_eof;
			return NULL;
		case -1: /* error */
			f->status = __FILE_err;
			return NULL;
	}

	/* FIXME: read only one line at a time */

	s[l] = '\0';

	return s;
}

/* error */
void perror(const char *s)
{
	fprintf(stderr, "%s%s%s\n",
			s ? s    : "",
			s ? ": " : "",
			strerror(errno));
}

/* file system */

int remove(const char *f)
{
	if(unlink(f)){
		if(errno == EISDIR)
			return rmdir(f);
		return -1;
	}
	return 0;
}

#undef fileno
int fileno(FILE *f)
{
	return f->fd;
}
