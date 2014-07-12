// RUN: %ocheck 0 %s

extern void abort(void);

f(int x)
{
	typedef short vla[x]; // TODO: check for multiple eval

	x = 1;

	vla a, b;

	return sizeof a + sizeof b;
}

main()
{
	if(f(3) != 2 * (3 * sizeof(short)))
		abort();

	return 0;
}
