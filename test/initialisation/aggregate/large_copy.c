// RUN: %ocheck 0 %s

main()
{
	struct A { int i, j, k; char abc; } ar[] = {
		[0 ... 3] = { 1, 2 }
	};

	for(int i = 0; i <= 3; i++)
		if(ar[i].i != 1 || ar[i].j != 2
		|| ar[i].k || ar[i].abc)
			abort();

	return 0;
}
