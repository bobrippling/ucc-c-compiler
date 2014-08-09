f(int ok)
{
	if(ok)
		return 1;

	return;
}

main()
{
	int x = f(1);
	int y = f(0);

	return x + y;
}
