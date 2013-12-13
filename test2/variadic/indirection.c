// RUN: %ocheck 7 %s

vf(__builtin_va_list l)
{
	return 2 + __builtin_va_arg(l, int);
}

f(int a, ...)
{
	__builtin_va_list l;
	__builtin_va_start(l, a);

	int r = vf(l);

	__builtin_va_end(l);

	return r;
}

main()
{
	return f(0, 5);
}
