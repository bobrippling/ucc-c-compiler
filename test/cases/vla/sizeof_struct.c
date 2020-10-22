// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

f(int n)
{
	struct A
	{
		int buf[5];
	} a[3][n];

	// ensure evaluating a[0] doesn't try to load a struct A
	return sizeof(a[0]);
}

main()
{
	if(f(2) != 5 * 2 * sizeof(int))
		abort();

	return 0;
}
