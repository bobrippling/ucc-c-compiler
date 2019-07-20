// RUN: %ocheck 0 %s
clean(int (*p)[2])
{
	void abort();
	int *ar = *p;
	if(ar[0] != 1)
		abort();
	if(ar[1] != 2)
		abort();
}

main()
{
	int x[2] __attribute((cleanup(clean))) = { 1, 2 };

	return 0;
}
