main()
{
	int *p = malloc(sizeof(int) * 4);
	int i;

	for(i = 0; i < 3; i++)
		p[i] = i;

	*p++ = 7;

	printf("p[1] = %d, p[3] = %d\n", p[1], p[3]);

	return 0;
}
