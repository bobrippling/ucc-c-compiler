// RUN: %ocheck 2 %s

int fn(int i)
{
	return i + 1;
}

main()
{
	int i = i;
	i = 0;
	__auto_type x = i;
	__auto_type f = fn;
	__auto_type f2 = &fn;

	return f2(f(x));
}
