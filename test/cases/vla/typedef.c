// RUN: %ocheck 0 %s

extern void abort(void);

gcalled;

g()
{
	if(!gcalled){
		gcalled = 1;
		return 0;
	}
	abort();
}

f(int x)
{
	typedef short vla[x + g()];

	x = 1;

	vla a, b;

	return sizeof a + sizeof b;
}

main()
{
	if(f(3) != 2 * (3 * sizeof(short)))
		abort();

	if(gcalled != 1)
		abort();

	return 0;
}
