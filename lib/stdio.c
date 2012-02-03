#include "unistd.h"
#include "stdio.h"
#include "string.h"
#include "errno.h"

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

static void printn(int fd, int n, int base)
{
	if(n < 0){
		write(fd, "-", 1);
		n = -n;
	}

	printd_rec(fd, n, base);
}

static void printd(int fd, int n)
{
	printn(fd, n, 10);
}

static void printx(int fd, int n)
{
	printn(fd, n, 16);
}


/* Public */
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
#ifdef PRINTF_OPTIMISE
			if(buflen)
				write(fd, buf, buflen);
#endif

			fmt++;

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
				case 'd':
					printd(fd, va_arg(ap, int));
					break;
				case 'p':
					write(fd, "0x", 2);
				case 'x':
					printx(fd, va_arg(ap, int));
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
	if(s)
		fprintf(stderr, "%s: ", s);
	fprintf(stderr, "%s\n", strerror(errno));
	/*fprintf(stderr, "%s%s%s\n",
			s ? s    : "",
			s ? ": " : "",
			strerror(errno)); - waiting for ?: bugfix */
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
