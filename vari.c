//#include <stdarg.h>

typedef __builtin_va_list va_list;

f(int a, ...)
{
	va_list l;

	__builtin_va_start(l, 5);
	//vf(a, l);

	return a + __builtin_va_arg(l, int);

	//va_end(l);
}

main()
{
	return f(1, 2);
}
