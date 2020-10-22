// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

gs, fs;

g()
{
	gs++;
	return 0;
}

f()
{
	fs++;
	return 2;
}

main()
{
	__typeof(  (short (**)[][f()]) g()  ) x;

	if(sizeof(x) != sizeof(short **))
		abort();

	if(sizeof(x) != sizeof(short **))
		abort();

	if(sizeof(short (**)[][f()]) != sizeof(short **))
		abort();

	x++; // ensures we can increment pointer to pointer

	return 0;
}
