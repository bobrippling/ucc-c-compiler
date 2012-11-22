p(int (*a)[2])
{
	int i, j;
	for(i = 0; i < 2; i++)
		for(j = 0; j < 2; j++)
			printf("[%d][%d] = %d\n", i, j, a[i][j]);
}

main()
{
	int a[][2] = { 0, 1, 2, 3 };
	int b[][2] = { {1}, {2} };
	int c[][2] = { {1}, 2, 3 };
	int d[][2] = { 1, 2, {3} };

	p(a);
	p(b);
	p(c);
	p(d);
}
