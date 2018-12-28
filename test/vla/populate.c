// RUN: %ocheck 0 %s

g()
{
	return 5;
}

populate(int *p, int n)
{
	while(n --> 0)
		*p++ = n;
}

sum(int *p, int n)
{
	int t = 0;
	while(n --> 0)
		t += *p++;
	return t;
}

f(int n)
{
	int x[n + g()];

	populate(x, sizeof x / sizeof *x);
	return sum(x, sizeof x / sizeof *x);
}

main()
{
	if(f(3) != 28)
		abort();

	return 0;
}
