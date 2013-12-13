main()
{
	struct { int i, j; } x[] = {
		[0 ... 5] = { 5, 6 },
		[3] = 1,
		[4] = { 3, 4 },
	};

	for(int i = 0; i < 6; i++)
		printf("[%d] = { %d, %d }\n", i, x[i].i, x[i].j);
}
