printd(int);

tim(int a, int b, int c)
{
	printd(a + b + c);
}

main()
{
	int i, j;

	i = 4;
	j = 2;

	{
		int k;
		k = 3;

		tim(i, j, k);
	}
}
