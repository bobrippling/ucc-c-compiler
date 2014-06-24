main()
{
	struct A
	{
		int a, b;
	} x, *p;
	int *pa, *pb;

	p = &x;

	pa = &x.a;
	pb = &p->a;

	*pa = 5;

	printf("%p, %p\n", pa, pb);

	return *pb == 5 ? 0 : 1;
}
