x(int x[2])
{
	int i;

	for(i = 0; i < 2; i++)
		printf("x[%d] = %d\n", i, x[i]);
}

main()
{
	int p[3];

	p[0] = 2;
	p[1] = 3;
	p[2] = 5;

	x(p + 1);
}
