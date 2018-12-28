// RUN: %ocheck 0 %s

f(int n)
{
	__typeof(short [2][n]) x;

	return sizeof x;
}

g(int n)
{
	long buf[2 * n];
	__typeof(buf) buf2;

	if(sizeof(buf2) != sizeof(buf))
		abort();

	return sizeof buf;
}

main()
{
	if(f(3) != sizeof(short) * 2 * 3)
		abort();

	if(g(3) != sizeof(long) * 2 * 3)
		abort();

	return 0;
}
