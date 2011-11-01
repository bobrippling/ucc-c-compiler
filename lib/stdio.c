#include "unistd.h"
#include "stdio.h"

static int _stdin  = 0;
static int _stdout = 1;
static int _stderr = 2;

FILE *stdin  = &_stdin;
FILE *stdout = &_stdout;
FILE *stderr = &_stderr;


/* Private */
static printd_rec(int n)
{
	int d, m;
	d = n / 10;
	m = n % 10;
	if(d)
		printd_rec(d);
	m = m + '0';
	write(1, &m, 1);
}

static printd(int n)
{
	if(n < 0){
		int neg;
		n = -n;
		neg = '-';
		write(1, &neg, 1);
	}

	printd_rec(n);
}

/* Public */

vfprintf(FILE *file, const char *fmt, va_list ap)
{
	int fd = *file;

	while(*fmt){
		if(*fmt == '%'){
			fmt++;

			if(*fmt == 's')
				write(1, *(char **)ap, strlen(*(char **)ap));
			else if(*fmt == 'c')
				write(1, ap, 1);
			else if(*fmt == 'd')
				printd(*(int *)ap);

			ap++;
		}else{
			write(1, fmt, 1);
		}
		fmt++;
	}
}

fprintf(FILE *file, const char *fmt, ...)
{
	return vfprintf(file, fmt, &fmt + 1);
}

printf(const char *fmt, ...)
{
	return vfprintf(stdout, fmt, &fmt + 1);
}
