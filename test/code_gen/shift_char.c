// RUN: %ocheck 0 %s

main()
{
	char c = 5;

	c <<= 1;

	if(c != 10)
		abort();

	return 0;
}
