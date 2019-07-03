// RUN: %ocheck 0 %s

f()
{
	struct A
	{
		int i, j;
	} a = {
	};

	if(a.i != 0 || a.j != 0)
		abort();

	struct A b = { 1, 2, 3 };

	if(b.i != 1 || b.j != 2)
		abort();
}

main()
{
	f();

	struct
	{
		int a, b, c;
	} x = { 1, 2 /* 3 */ };

	if(x.c != 0)
		abort();

	return 0;
}
