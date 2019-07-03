// RUN: %ocheck 0 %s

//#include <stdarg.h>
typedef __builtin_va_list va_list;
#define va_start(l, p) __builtin_va_start(l, p)
#define va_arg(l, ty)  __builtin_va_arg(l, ty)
#define va_end(l)      __builtin_va_end(l)

float f(int a, ...)
{
	va_list l;
	va_start(l, a);
	float r = va_arg(l, double);
	va_end(l);
	return r;
}

main()
{
	float x = f(2, (double)5);

	if(x != 5)
		abort();
	return 0;
}
