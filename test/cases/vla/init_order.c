// RUN: %ocheck 0 %s
abort();
n;

int a()
{
	if(n++)
		abort();
	return 2;
}

int b()
{
	if(n++ != 1)
		abort();
	return 42;
}

int c()
{
	if(n++ != 2)
		abort();
	return 92;
}

main()
{
	// should be initialised before the vla is created
	int a_ = a();
	short ar[b()];
	int c_ = c();

	if(n != 3)
		abort();
	if(a_ != 2)
		abort();
	if(sizeof(ar) != 42 * sizeof(short))
		abort();
	if(c_ != 92)
		abort();

	return 0;
}
