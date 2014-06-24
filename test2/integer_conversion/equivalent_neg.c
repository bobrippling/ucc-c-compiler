// RUN: %ocheck 0 %s

main()
{
	int x = -1;
	int y = 4294967295;

	if(x != y)
		abort();

	return 0;
}
