// RUN: %ocheck 0 %s


f_calls;

f()
{
	f_calls++;
	return 7;
}

struct A
{
	int i, j, k;
};
check(const struct A *p, int a, int b, int c)
{
	if(p->i != a || p->j != b || p->k != c)
		abort();
}

main()
{
	struct A x[] = {
		{ 1, 2, 3 },
		{ 1, 2, 3 },
		[1 ... 3] = { f(), 5, 6 }
	};

	if(f_calls != 1)
		abort();

	check(&x[0], 1, 2, 3);
	check(&x[0], 1, 2, 3);
	check(&x[1], 7, 5, 6);
	check(&x[2], 7, 5, 6);
	check(&x[3], 7, 5, 6);
	return 0;
}
