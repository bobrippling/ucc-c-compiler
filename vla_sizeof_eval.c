f(int n)
{
	struct A
	{
		char buf[1024];
	} a[1][n];

	// ensure evaluating a[0] doesn't try to load a struct A
	return sizeof(a[0]);
}
