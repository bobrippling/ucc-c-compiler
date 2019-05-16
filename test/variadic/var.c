// RUN: %ocheck 8 %s

f(int a, ...)
{
	__builtin_va_list l;
	int ret;

	__builtin_va_start(l, a);

	__builtin_va_arg(l, int);
	ret = a + __builtin_va_arg(l, int);
	__builtin_va_end(l);

	return ret;
}

main()
{
	return f(5, 2, 3, 4);
}
