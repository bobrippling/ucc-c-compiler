static int *f(int p[])
{
	return p + 1;
}

main()
{
	int i[2];
	int *p = f(i);

	*p = 3;

	return i[1];
}
