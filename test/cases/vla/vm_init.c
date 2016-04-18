// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

syntax(int n)
{
	int ar[n];
	int (*p)[n] = &ar;

	syntax(p);
}

assert(_Bool b)
{
	if(!b)
		abort();
}

f(int n)
{
	short (*p)[n] = 0;

	assert(p == 0);
	assert((int)(p + 1) == 3 * sizeof(short));
}

main()
{
	f(3);

	return 0;
}
