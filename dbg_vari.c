typedef __builtin_va_list va_list;

#define va_start(l, arg) __builtin_va_start(l, arg)
#define va_arg(l, type)  __builtin_va_arg(l, type)
#define va_end(l)        __builtin_va_end(l)

sum(int a, ...)
{
	va_list l;
	va_start(l, a);

	int t = 0; // BUG: 't' can't be printed - bad SLEB128
	for(t += a;
			a > 0;
			a = va_arg(l, int))
		;

	va_end(l);
	return t;
}

main()
{
	printf("%d\n", sum(1, 2, 3, 0));
}
