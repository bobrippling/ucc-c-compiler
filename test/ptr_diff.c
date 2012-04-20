main()
{
	int a, b;
	int *pa, *pb;

	pa = &a;
	pb = &b;

	assert(pb - pa == 1);
}
