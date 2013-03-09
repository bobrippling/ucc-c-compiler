main()
{
	int *p = (int[]){1,2,3};
	struct A { int i, j, k[2]; } a = {
		.k[2] = 3,
		.j = 3,
	};

	return p[1] + a.k[1] + a.k[2] + a.j;
}
