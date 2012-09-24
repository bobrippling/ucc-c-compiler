main()
{
	int x[][2] = {
		1, { 2, 3 }, 4 // 1, 2, 4, 0
	};

	int i, j;
	for(i = 0; i < 4; i++)
		for(j = 0; j < 2; j++)
			printf("x[%d][%d] = %d\n", i, j, x[i][j]);
}
