// RUN: %ocheck 0 %s

union A
{
	struct
	{
		int i, j, k;
	} a;
	int i;
	char *p;
};

union_ar()
{
	union A ar[] = {
		{ 1 }, // [0]
		2, 3 // [1], i & j
	};

	if(sizeof(ar)/sizeof(*ar) != 2)
		abort();
	if(sizeof(ar) != 2 * 16) // 12, padded to 16 for .p alignment
		abort();

	if(ar[0].a.i != 1
	|| ar[0].a.j
	|| ar[0].a.k)
		abort();

	if(ar[1].a.i != 2
	|| ar[1].a.j != 3
	|| ar[1].a.k)
		abort();
}

union_full_zero()
{
	union A b = {};

	if(b.a.i || b.a.j || b.a.k)
		abort();
	if(b.i)
		abort();
	if(b.p)
		abort();
}

union_init()
{
	union A union_single_init = {
		.i = 1, 2, 3 // 2 & 3 ignored
	}, union_zero_rest_init = {
		.a.i = 1, // a.[jk] = 0
	};

	if(sizeof(union_single_init) != sizeof(__typeof(union_single_init)))
		abort();
	if(sizeof(union_single_init) != sizeof(union A))
		abort();

	if(union_single_init.i != 1)
		abort();
	if(union_zero_rest_init.a.i != 1
	|| union_zero_rest_init.a.j
	|| union_zero_rest_init.a.k)
		abort();
}

main()
{
	union_init();
	union_full_zero();
	union_ar();
}
