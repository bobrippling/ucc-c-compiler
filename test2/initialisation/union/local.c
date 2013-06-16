// RUN: %ucc -o %t %s
// RUN: %t | diff -u - %s.out

#define PEXP(fmt, exp) printf(#exp " = " fmt "\n", (exp))
main()
{
	union A
	{
		struct
		{
			int i, j, k;
		} a;
		int i;
		char *p;
	} x = { .i = 1, 2, 3 };

	int y = sizeof(x);

	union A ar[] = { { 1 }, 2, 3 };

	union A b = {};

	printf("x = { .i = %d } :: { %d, %d, %d }\n",
			x.i, x.a.i, x.a.j, x.a.k);

	PEXP("%d", y);
	PEXP("%d", sizeof(ar)/sizeof(*ar));

	printf("ar[0].a = { %d, %d, %d }\n",
			ar[0].a.i, ar[0].a.j, ar[0].a.k);

	printf("ar[1].a = { %d, %d, %d }\n",
			ar[1].a.i, ar[1].a.j, ar[1].a.k);

	printf("b = { .a = { %d, %d, %d }, .i = %d, .p = %p }\n",
			b.a.i, b.a.j, b.a.k, b.i, b.p);
}
