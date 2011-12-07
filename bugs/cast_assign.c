main()
{
	int i;
	int p;

	p = (int)&i;

	*(char *)p = 5;

	return i;
}
