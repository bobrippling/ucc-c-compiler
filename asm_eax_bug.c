#include <stdarg.h>

int r = 5;

pf(int i, ...)
{
	va_list l;

	va_start(l, i);
	r = va_arg(l, int);
	va_end(l);
	return r;
}

main()
{
	return pf(1, 2);
}
