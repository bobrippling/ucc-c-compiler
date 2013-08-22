// RUN: %ucc -o %t %s
// RUN: %t

#include <stdarg.h>

float f(int a, ...)
{
	va_list l;
	va_start(l, a);
	float r = va_arg(l, float);
	va_end(l);
	return r;
}

main()
{
	float x = f(2, (float)5);

	if(x == 5)
		return 0;
	return 7;
}
