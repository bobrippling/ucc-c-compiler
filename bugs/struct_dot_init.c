struct
{
	int i, j;
} a = { 0, 1 };

main()
{
	struct
	{
		int i, j;
	} a = {
		.i = 1,
		.j = 2
	};

	return a.i + a.j;
}
