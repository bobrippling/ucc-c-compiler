f(int i)
{
	return 3 + i;
}

g(int i)
{
	return 1 - g(i);
}

main()
{
	return g(7);
}
