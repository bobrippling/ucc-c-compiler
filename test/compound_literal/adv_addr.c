// RUN: %ocheck 2 %s

main()
{
	int (*p)[];

	p = &(int[]){1, 2};

	return (*p)[1];
}
