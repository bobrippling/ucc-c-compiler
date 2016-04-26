// RUN: %ocheck 3 %s

#include <stdarg.h>

f()
{
	int x = 3;
	va_list l;
	va_start(l, x); // shouldn't crash here
	// (despite being invalid code)
	int r = va_arg(l, int);
	va_end(l);
	return r;
}

g(x)
	int x;
{
	va_list l;
	va_start(l, x); // shouldn't crash here
	int r = va_arg(l, int);
	va_end(l);
	return r + x;
}

main()
{
	return g(1, 2);
}
