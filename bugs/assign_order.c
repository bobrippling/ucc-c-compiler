int f()
{
	return 3;
}

main()
{
	int i;
	i = f() > 2; // i becomes 3, not 1
	return i;
}
