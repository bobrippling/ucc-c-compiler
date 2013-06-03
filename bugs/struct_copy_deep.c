main()
{
	struct A
	{
		int i, j;
	};

	struct A a, b, c = { 1, 2 };

	a = b = c;
}
