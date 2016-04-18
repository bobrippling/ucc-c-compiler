// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

fs;
f()
{
	fs++;
	return 1;
}

test()
{
	short (**x)[][f()];
}

main()
{
	test();

	if(fs != 1)
		abort();

	return 0;
}
