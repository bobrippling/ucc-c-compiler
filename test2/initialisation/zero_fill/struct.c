main()
{
	struct
	{
		int a, b, c;
	} x = { 1, 2 /* 3 */ };
}

f()
{
	struct A
	{
		int i, j;
	} a = {
	};

	struct A b = { 1, 2, 3 };
}
