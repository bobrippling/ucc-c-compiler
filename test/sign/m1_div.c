// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

unsigned long foo()
{
	return (unsigned long) - 1 / 8;
	// ((unsigned long) -1) / 8
}

main()
{
	if(foo() == 0)
		abort();
	return 0;
}

