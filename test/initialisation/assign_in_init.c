// RUN: %ocheck 0 %s

main()
{
	struct A
	{
		int i, j, k;
	} x = {
		.i = 5,
		.j = 2,
		x.k = 3,
		// order is implementation specific, but doesn't matter here
	};

	if(x.i != 5 || x.j != 2 || x.k != 3)
		abort();

	return 0;
}
