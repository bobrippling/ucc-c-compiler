f(int *i)
{
	*i = 3;
}

main()
{
	int i;
	f(&i);
	while(i == 3)
		i = 2;
	i = 4 / 2;
	return i;
}
