// RUN: %ocheck 0 %s

main()
{
	int y[][2] = {
		1, 2,
		3, 4
	};

	if(y[0][0] != 1
	|| y[0][1] != 2
	|| y[1][0] != 3
	|| y[1][1] != 4)
	{
		abort();
	}

	return 0;
}
