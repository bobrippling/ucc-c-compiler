main()
{
	int *p = malloc(sizeof(int) * 4);
	int i;

	for(i = 0; i < 3; i++)
		p[i] = i;

	*p++ = 7;

	printf("p[3] = %d, p[9] = %d\n", p[3], p[9]);

	return 0;
}
