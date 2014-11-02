main()
{
	int i = 2, j = 7;

	__asm("0=%0 1=%1" : "=r"(i) : "0"(j));

	// j -> memory @ xyz
	// i <- memory @ xyz
	__asm("0=%0 1=%1" : "=m"(i) : "0"(j));

	__asm("0=%0 1=%1" : "=g"(i) : "0"(j));

	//__asm("0=%0 1=%1" : "=g"(*(int *)(0 == 0)) : "0"(j));

	return i;
}
