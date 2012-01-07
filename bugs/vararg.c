#include <stdarg.h>

va(int a, ...)
{
	va_list l;

	va_start(l, a);
	do{
		printf("%d\n", a);
		a = va_arg(l, int);
	}while(a);
	va_end(l);
}

main()
{
	va(1, 2, 3, 0);
}
