// RUN: %ucc -g %s -o %t

typedef __builtin_va_list va_list;

#define va_start(l, arg) __builtin_va_start(l, arg)
#define va_arg(l, type)  __builtin_va_arg(l, type)
#define va_end(l)        __builtin_va_end(l)

sum(int a, ...)
{
	va_list l;
	va_start(l, a);

	int t = 0;
	for(t += a;
			a > 0;
			a = va_arg(l, int))
		;

	va_end(l);
	return t;
}

main()
{
	return sum(1, 2, 3, 0);
}
