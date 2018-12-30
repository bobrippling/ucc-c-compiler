// RUN: %ocheck 0 %s
main()
{
	int i = -5 % 2;

	if(i != -1)
		abort();

	return 0;
}
