// RUN: %ocheck 0 %s
f()
{
	int x[2];
	if(_Generic(x, int *: 1, int[2]: 3) != 3)
		abort();
}

g()
{
	int x[2];
	if(_Generic((0, x), int *: 1, int[2]: 3) != 1)
		abort();
}

main()
{
	f();
	g();
	return 0;
}
