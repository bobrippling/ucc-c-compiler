// RUN: %ucc -o %t %s
// RUN: %t | %output_check '^1 2 0 0$' '^1 2 0 0$' '^1 2 0 0$' '^1 2 0 0$'

main()
{
	struct A { int i, j, k; char abc; } ar[] = {
		[0 ... 3] = { 1, 2 }
	};

	for(int i = 0; i <= 3; i++)
		printf("%d %d %d %d\n",
				ar[i].i,
				ar[i].j,
				ar[i].k,
				ar[i].abc);
}
