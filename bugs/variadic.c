printf(char *, ...) __attribute((format(printf, 1, 2)));

double sum(double d, ...)
{
	__builtin_va_list l;

	__builtin_va_start(l, d);
	double t = 0;
	while(d){
		t += d;
		d = __builtin_va_arg(l, double);
	}
	__builtin_va_end(l);
	return t;
}

main()
{
	printf("%f\n", sum(1, 2.3, 5.1, 0.0)); // 8.4
}
