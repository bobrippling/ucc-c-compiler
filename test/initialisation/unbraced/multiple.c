// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

f()
{
	int x[][2] = { 1, 2, 3, 4 };

	if(x[0][0] != 1
	|| x[0][1] != 2
	|| x[1][0] != 3
	|| x[1][1] != 4)
		abort();
}

g()
{
	struct
	{
		int i;
		struct
		{
			int j, k;
		} a;
	} x = { 1, 2, 3 };

	if(x.i != 1 || x.a.j != 2 || x.a.k != 3)
		abort();
}

main()
{
	f();
	g();
	return 0;
}
