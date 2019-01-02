// RUN: %ocheck 0 %s

main()
{
	volatile int x = L'\x0000000100';

	if(x != 0x100)
		abort();

	return 0;
}
