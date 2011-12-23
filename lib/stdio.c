#include "unistd.h"
#include "stdio.h"
#include "string.h"

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
	write(fd, "0x", 2);
	printn(fd, n, 16);
}


/* Public */

int vfprintf(FILE *file, char *fmt, va_list ap)
{
	int fd = *file;
#ifdef PRINTF_OPTIMISE
	char *buf  = fmt;
	int buflen = 0;
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
					char *s = *(char **)ap;
					if(!s)
						s = "(null)";
					write(fd, s, strlen(s));
					break;
				}
				case 'c':
					write(fd, (char *)ap, 1);
					break;
				case 'd':
					printd(fd, *(int *)ap);
					break;
				case 'p':
				case 'x':
					printx(fd, *(int *)ap);
					break;

				default:
					write(fd, fmt, 1); /* default to just printing the char */
			}

#ifdef PRINTF_OPTIMISE
			buf = fmt + 1;
			buflen = 0;
#endif

			ap += sizeof(void *); /* void arith, use pointer size */
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
	return vfprintf(&fd, fmt, (void *)(&fmt + 1));
}

int fprintf(FILE *file, const char *fmt, ...)
{
	return vfprintf(file, fmt, (void *)(&fmt + 1));
}

int printf(const char *fmt, ...)
{
	return vfprintf(stdout, fmt, (void *)(&fmt + 1));
}

int puts(const char *s)
{
	return printf("%s\n", s) > 0 ? 1 : -1;
}
