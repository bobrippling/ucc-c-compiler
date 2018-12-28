main()
{
	int x = 5;
	void *p = &x;
	*(int *)p = 3;
	return x;
}
