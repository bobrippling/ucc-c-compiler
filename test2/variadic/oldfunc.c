// RUN: %ocheck 0 %s

#include <stdarg.h>

int f(anchor)
	int anchor;
{
	va_list l;

	va_start(l, anchor);
	int r = (va_arg(l, int) != 5);
	va_end(l);

	return r;
}

main()
{
	return f(0, 5);
}

