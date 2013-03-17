main()
{
	const int i = 5;
	const int x[] = { 1, 2, 3 };
	const struct
	{
		int i, j;
		char *s;
	} y[] = {
		1, 2, { "hi" },
		{ 2 },
		{ 5, 6, "yo" }
	};
}
