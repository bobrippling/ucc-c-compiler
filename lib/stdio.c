#include "unistd.h"
#include "stdio.h"
#include "string.h"
#include "errno.h"
#include "stdlib.h"
#include "sys/fcntl.h"
#include "assert.h"
#include "ctype.h"

#define PRINTF_OPTIMISE

static int _stdin  = 0;
static int _stdout = 1;
static int _stderr = 2;

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


/* Public */
FILE *fopen(const char *path, char *smode)
{
	FILE *f;
	int fd, mode;
	int got_primary;

#define PRIMARY_CHECK() \
	if(got_primary) \
		goto inval; \
	got_primary = 1

	got_primary = mode = 0;
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
				return NULL;
		}
	if(!mode)
		goto inval;

#undef PRIMARY_CHECK

	f = malloc(sizeof *f);
	if(!f)
		return NULL;

	fd = open(path, mode, 0644);
	if(fd == -1){
		free(f);
		return NULL;
	}

	*f = fd;

	return f;
}

int fclose(FILE *f)
{
	int r = close(*f) == 0 ? 0 : EOF;
	free(f);
	return r;
}

int fputc(int c, FILE *f)
{
	return write(*f, &c, 1) == 1 ? c : EOF;
}

int putchar(int c)
{
	return fputc(c, stdout);
}

int vfprintf(FILE *file, char *fmt, va_list ap)
{
	int fd = *file;
#ifdef PRINTF_OPTIMISE
	char *buf  = fmt;
	int buflen = 0;
#else
# warning printf unoptimised
#endif

	while(*fmt){
		if(*fmt == '%'){
			int pad = 0;

#ifdef PRINTF_OPTIMISE
			if(buflen)
				write(fd, buf, buflen);
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
					char *s = va_arg(ap, char *);
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
						fputs("0x", file);
						printx(fd, p, 0);
					}else{
						fputs("(nil)", file);
					}
					break;
				}
				case 'x':
					printx(fd, va_arg(ap, int), 1);
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

	va_start(l, fmt);
	r = vfprintf(&fd, fmt, l);
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
	return write(*f, s, strlen(s)) > 0 ? 1 : EOF;
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
	return read(*f, &ch, 1) == 1 ? ch : EOF;
}

char *fgets(char *s, int l, FILE *f)
{
	int r;

	r = read(*f, s, l - 1);

	switch(r){
		case  0: /* EOF */
		case -1: /* error */
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
