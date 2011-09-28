#include "unistd.h"

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

int printf(const char *fmt, ...)
{
	void *ap = &fmt + sizeof int;

	while(*fmt){
		if(*fmt == '%'){
			fmt++;
			/**/ if(*fmt == 's') write(1, *(char **)ap, strlen(*(char **)ap));
			else if(*fmt == 'c') write(1, *(char  *)ap, 1);
			else if(*fmt == 'd') printd(  *(int   *)ap);
			ap = ap + sizeof int;
		}else{
			write(1, *fmt, 1);
		}
		fmt++;
	}
}
