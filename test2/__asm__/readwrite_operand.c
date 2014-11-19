// RUN: %ocheck 7 %s

assert(_Bool b)
{
	extern void abort(void);
	if(!b)
		abort();
}

main()
{
	int i = 3;

	__asm("incl %0" : "+r"(i));
	assert(i == 4);
	__asm("addl %1, %0" : "+r"(i) : "g"(3));
	assert(i == 7);

	return i;
}
