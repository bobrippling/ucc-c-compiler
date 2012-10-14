main()
{
	int x[][2] = {
		1,
#define SECOND
#ifdef SECOND
		2,
#endif
		{ 3, 4 }
	};

	for(int i = 0; i < 2; i++)
		for(int j = 0; j < 2; j++)
			printf("x[%d][%d] = %d\n", i, j, x[i][j]);
}
