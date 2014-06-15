// RUN: %ocheck 0 %s

extern _Noreturn void abort(void);

int abort2()
{
	abort();
}

main()
{
	if(_Generic(abort2(), int: 0))
		abort();

	if(_Generic(0, void *: abort2(), int: 3) != 3)
		abort();

	int x = 2;
	if(_Generic(0, int: x = 3) != 3)
		abort();
	if(x != 3)
		abort();

	const char k = 0; /* test on char - no decay/promotion */
	if(_Generic(k, char: 0, default: abort2()))
		abort();
	if(_Generic((const char)0, char: 0)) // XXX
		abort();

	int ar[1];
	const int kar[1];
	if(_Generic(ar, int *: 0, const int *: abort2())) // XXX
		abort();
	if(_Generic(kar, const int *: 0, int *: abort2())) // XXX
		abort();

	if(_Generic(main, int (*)(): 0)) // XXX
		abort();

	return 0;
}
