// RUN: %ocheck 0 %s

main()
{
	unsigned char a = 0xff;
	char b = 0xff;
	int c = a == b;

	if(c)
		abort();

	return 0;
}
