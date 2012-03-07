f(int *i)
{
	*i = 3;
}

main()
{
	int i;
	f(&i);
	return i;
}
