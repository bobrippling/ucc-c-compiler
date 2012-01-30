x(int x[2])
{
	int i;

	for(i = 0; i < 2; i++)
		printf("x[%d] = %d\n", i, x[i]);
}

main()
{
	int p[2];

	*p = 2;
	p[1] = 3;

	x(p);
}
