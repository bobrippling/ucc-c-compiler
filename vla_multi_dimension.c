void abort(void);

g() { return 3; }
h() { return 2; }

void init(int a, int b, int (*p)[a])
{
	for(int i = 0; i < a; i++)
		for(int j = 0; j < b; j++)
			p[i][j] = i + j;
}

f(int n)
{
	int ar[n * g()][h()];

	if(sizeof ar != h() * g() * sizeof(int))
		abort();

	if(sizeof *ar != h() * sizeof(int))
		abort();

	init(sizeof ar / sizeof *ar, sizeof *ar, ar);

	for(int i = 0; i < (int)(sizeof ar / sizeof *ar); i++)
		for(int j = 0; j < (int)(sizeof *ar / sizeof **ar); j++)
			if(ar[i][j] != i + j)
				abort();

	return sizeof ar;
}

main()
{
	int sz = f(3);

	if(sz != 72)
		abort();

	return 0;
}
