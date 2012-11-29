f()
{
	int x[][2] = { 1, 2, 3, 4 };
}

g()
{
	struct
	{
		int i;
		struct
		{
			int j, k;
		} a;
	} x = { 1, 2, 3 };
}
