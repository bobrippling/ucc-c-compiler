// RUN: %ocheck 6 %s

sumv(int a, __builtin_va_list l)
{
	int t = 0;
	while(a){
		t += a;
		a = __builtin_va_arg(l, int);
	}
	return t;
}

sum(int a, ...)
{
	__builtin_va_list l;
	__builtin_va_start(l, a);
	int const t = sumv(a, l);
	__builtin_va_end(l);
	return t;
}

main()
{
	return sum(1, 2, 3, 0);
}
