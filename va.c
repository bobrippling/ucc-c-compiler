f(int i, ...)
{
	write(2, "1\n", 2);
	__builtin_va_list l;
	write(2, "2\n", 2);
	__builtin_va_start(l, 0);
	write(2, "3\n", 2);
	__builtin_va_arg(l, int);
	write(2, "4\n", 2);
}

main()
{
	write(2, "0\n", 2);
	return f(1, 2, 3, 0);
}
