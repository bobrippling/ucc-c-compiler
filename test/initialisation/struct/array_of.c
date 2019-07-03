// RUN: %ocheck 0 %s

main()
{
	struct
	{
		int i, j;
	} a[] = {
		{ 1, 2 },
		{ 3, 4 }
	};

	if(a[0].i != 1
	|| a[0].j != 2
	|| a[1].i != 3
	|| a[1].j != 4)
	{
		abort();
	}

	return 0;
}
