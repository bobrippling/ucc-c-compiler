// RUN: %ucc -fsyntax-only %s

#include <stdarg.h>

f()
{
	int x = 3;
	va_list l;
	va_start(l, x); // shouldn't crash here
	int r = va_arg(l, int);
	va_end(l);
	return r;
}

main()
{
	printf("%d\n", f(1, 2));
}
