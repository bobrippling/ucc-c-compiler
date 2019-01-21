// RUN: %ocheck 0 %s

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

