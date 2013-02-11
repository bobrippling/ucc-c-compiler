int h[] = {
	{{ 1 }}, 2
};

int g[][2] = { {1}, 2, 3, {4}, 5, {6}, {7} };

main()
{
	int y, x;
	for(y = 0; y < sizeof(g)/sizeof(*g); y++)
		for(x = 0; x < 2; x++)
			printf("y[%d][%d] = %d\n", y, x, g[y][x]);
}
