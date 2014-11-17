// RUN: %ocheck 0 %s -std=c89

#ifdef __STDC_VERSION__
#error need C89
#endif

g()
{
	static int called;
	if(called)
		abort();
	called = 1;
	return 1;
}

main()
{
	int i = g();

	if(i != 1)
		abort();

	return 0;
}
