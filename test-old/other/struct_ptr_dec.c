print_ar(int *p, int n)
{
	for(int i = 0; i < n; i++)
		printf("[%d] = %d\n", i, p[i]);
}

main()
{
	struct
	{
		int i;
	} ar[5];
	__typeof(*ar) *ar_p;
	int *p = ar;

	ar_p = ar + 1;

	for(int i = 0; i < 5; i++)
		p[i] = i;

	ar_p--->i = 8;
	ar_p->i = 9;

	print_ar(p, 5);
}
