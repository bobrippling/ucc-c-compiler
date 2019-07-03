// RUN: %ocheck 5 %s

main()
{
	int a;
	__asm("" : "=r"(a) : "r"(5));
	return a;
}
