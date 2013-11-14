// RUN: %ocheck 0 %s

char g[1];
const char *f()
{
	return g + 7;
}

main()
{
	int *p = g;
	if(f() != p + 7)
		abort();
	return 0;
}
