f(int i, int *p)
{
	*p = 1;
	return i + 1;
}

main()
{
	int i, j;
	i = f(3, &j);
	return i + 4 + j;
}
