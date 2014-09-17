// RUN: %ocheck 0 %s

void check(int result)
{
	if(result != -1)
		abort();
}

main()
{
	int i = -5;

	check((char)i % (char)2);
	check((short)i % (short)2);
	check(i % 2);
	check((long)i % 2);

	return 0;
}
