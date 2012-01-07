#include <stdarg.h>

va(int a, ...)
{
	va_list l;

	va_start(l, a);
	while(a){
		printf("%d\n", a);
		a = va_arg(l, int);
	}
	va_end(l);
}

main()
{
	va(0, 1);
	va(1, 2, 3, 0);
	va(4, 5, 6, 7, -1, 0);
}
