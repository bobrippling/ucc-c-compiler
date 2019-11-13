// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

f(int x)
{
	short buf[x];

	return sizeof buf;
}

g(int x)
{
	short buf[x];
	return sizeof(long [x + 1]) // 8 * (3 + 1) = 32
		+ sizeof buf; // 2 * 3 = 6
}

fatal(void)
{
	abort();
}

h()
{
	return __alignof(int [fatal()]);
}

main()
{
	if(f(3) != 3 * sizeof(short))
		abort();

	if(g(3) != 3 * sizeof(short) + sizeof(long) * 4)
		abort();

	if(h() != _Alignof(int))
		abort();

	return 0;
}
