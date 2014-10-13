// RUN: %ocheck 0 %s

static int x;

f()
{
	x = 5;

	static int x;

	x = 3;
}

main()
{
	f();
	if(x != 5)
		abort();
	return 0;
}
