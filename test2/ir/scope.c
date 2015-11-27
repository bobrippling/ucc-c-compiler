main()
{
	int i = 5;
	{
		int i = 3;
		f(i);
	}
	{
		int i = 2;
		f(i);
	}
	({
		int i = 2;
		f(i);
	});
}
