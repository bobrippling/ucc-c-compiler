#include <stdarg.h>

f(int a, ...)
{
	va_list l;
	va_start(l, a);
	return a + va_arg(l, int);
}

main()
{
	return f(1, 2);
}
