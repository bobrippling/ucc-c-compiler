// RUN: %ocheck 8 %s
main()
{
	int x[] = {
		[0 ... 9] = 3,
		[3      ] = 2
	};

	return x[3] + x[2] + x[4];
}
