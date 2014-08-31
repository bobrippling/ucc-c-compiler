main()
{
	int a;
	__asm("" : "=a"(a) : "a"(5));
	return a;
}
