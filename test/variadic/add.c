// RUN: %ocheck 10 %s

f(int a, ...)
{
	__builtin_va_list l;
	int ret = a;

	__builtin_va_start(l, a);

	for(;;){
		int i = __builtin_va_arg(l, int);
		if(i == 0)
			break;
		ret += i;
	}

	__builtin_va_end(l);

	return ret;
}

main()
{
	return f(5, 2, 3, 0);
}
