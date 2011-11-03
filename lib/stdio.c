#include "unistd.h"
#include "stdio.h"
#include "string.h"

static int _stdin  = 0;
static int _stdout = 1;
static int _stderr = 2;

FILE *stdin  = &_stdin;
FILE *stdout = &_stdout;
FILE *stderr = &_stderr;


/* Private */
static void printd_rec(int fd, int n)
{
	int d, m;
	d = n / 10;
	m = n % 10;
	if(d)
		printd_rec(fd, d);
	m = m + '0';
	write(fd, &m, 1);
}

static void printd(int fd, int n)
{
	if(n < 0){
		int neg;
		n = -n;
		neg = '-';
		write(fd, &neg, 1);
	}

	printd_rec(fd, n);
}

/* Public */

int vfprintf(FILE *file, const char *fmt, va_list ap)
{
	int fd = *file;

	while(*fmt){
		if(*fmt == '%'){
			fmt++;

			if(*fmt == 's')
				write(fd, *(char **)ap, strlen(*(char **)ap));
			else if(*fmt == 'c')
				write(fd, *(char *)ap, 1);
			else if(*fmt == 'd')
				printd(fd, *(int *)ap);

			ap++;
		}else{
			write(fd, fmt, 1);
		}
		fmt++;
	}
}

int fprintf(FILE *file, const char *fmt, ...)
{
	return vfprintf(file, fmt, &fmt + 1);
}

int printf(const char *fmt, ...)
{
	return vfprintf(stdout, fmt, &fmt + 1);
}
