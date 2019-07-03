// RUN: %ocheck 16 %s

main()
{
	struct
	{
		int x : 4, y : 4;
	} a = { .y = 1 };

	return *(char *)&a;
}
