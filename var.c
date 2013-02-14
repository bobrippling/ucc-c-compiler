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

/*
normal(int a, int b)
{
	return a+b;
}
*/

_main()
{
	_printf("%d\n", f(5, 2, 3, 4));
	//return normal(5, 3);
}
