// RUN: %ocheck 0 %s

f()
{
	int i = '\n' ^ 0;

	return i | ~1 + 0[""];
}

main()
{
	if(f() != -2)
		abort();

	return 0;
}
