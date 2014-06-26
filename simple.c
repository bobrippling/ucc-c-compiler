g(long *p)
{
	return p[1];
}

setup(long *p)
{
	p[1] = 7;
}

f(int n)
{
	long ar[3 * n]; // 8 * 2 * 3 = 48

	setup(ar);

	return g(ar);
}

main()
{
	return f(2);
}
