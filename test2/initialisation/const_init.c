// RUN: %ocheck 0 %s

main()
{
	const int i = 5;
	const int x[] = { 1, 2, 3 };
	const struct
	{
		int i, j;
		char *s;
	} y[] = {
		1, 2, { "hi" },
		{ 2 },
		{ 5, 6, "yo" }
	};

	if(i != 5 || x[0] != 1 || x[1] != 2 || x[2] != 3)
		abort();

	if(y[0].i != 1 ||  y[0].j != 2 || strcmp(y[0].s, "hi"))
		abort();
	if(y[1].i != 2 ||  y[1].j != 0 || y[1].s != 0)
		abort();
	if(y[2].i != 5 ||  y[2].j != 6 || strcmp(y[2].s, "yo"))
		abort();

	return 0;
}
