// RUN: %ocheck 0 %s

f()
{
	return 1;
}

void g()
{
}

main()
{
	// contains a vla but isn't evaluated
	int sz = sizeof *( 0 ? 0 : (int(*)[f()]) 0 );

	g(sz);

	return 0;
}
