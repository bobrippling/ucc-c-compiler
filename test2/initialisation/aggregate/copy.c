main()
{
	struct { int i, j; } x[] = {
		[1 ... 5] = { 1, 2 } // a memcpy is performed for these structs
	};

	for(int i = 0; i < 6; i++)
		printf("x[%d] = { %d, %d }\n",
				i, x[i].i, x[i].j);
}
