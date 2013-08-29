// RUN: %ocheck 0 %s

float f(int a, ...)
{
	double tot = a;

	__builtin_va_list l;
	__builtin_va_start(l, a);

	tot += __builtin_va_arg(l, double);
	tot += __builtin_va_arg(l, double);
	tot += __builtin_va_arg(l, int);

	__builtin_va_end(l);

	return tot;
}

main()
{
	float t = f(1,        // int
			(float)2, // cast to double
			3.2,      // double
			4);       // int

	if(t == 10.2f)
		return 0;

	return 5;
}
