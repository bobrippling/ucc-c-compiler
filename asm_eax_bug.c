second(int i, ...)
{
	void *l;
	int r;

	l = (void *)&i;
	r = (*(int *)(l += 8));
	return r;
}

main()
{
	return second(9, 3);
}
