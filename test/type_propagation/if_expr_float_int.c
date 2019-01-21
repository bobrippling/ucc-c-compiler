// RUN: %ocheck 0 %s

f(int b)
{
	return b ? 5 : 2.3;
}

main()
{
	if(f(0) != 2)
		abort();
	if(f(1) != 5)
		abort();

	return 0;
}
