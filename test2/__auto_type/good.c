// RUN: %ocheck 34 %s

int f(void)
{
	return 3;
}

main()
{
	__auto_type x = f();

	// this puts us into parse_btype()
	const __auto_type y = f();

	_Noreturn __auto_type noret = 5;
	__attribute__((unused)) __auto_type unused = 2;
	static __auto_type st = 10;
	_Alignas(long) __auto_type int_as_long = 10;

	__auto_type const after = 1;

	return x + y + noret + unused + st + int_as_long + after;
}
