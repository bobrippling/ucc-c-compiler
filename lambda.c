int indirect(int fn(int), int arg)
{
	return fn(arg);
}

main()
{
	int x = 3;

	int add(int a)
	{
		/*
		int add2(int q)
		{
			return x + q;
		}
		return indirect(add2, x + a);
		*/
		return x + a;
	}

	int total = indirect(add, 5);

	printf("%d\n", total);
}
