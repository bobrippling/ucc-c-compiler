// RUN: %ucc -o %t %s
// RUN: %t; [ $? -eq 8 ]

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
