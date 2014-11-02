main()
{
	int i = 3;

	__asm("incl %0" : "+r"(i));
	//__asm("addl %1, %0" : "+r"(i) : "r"(3));

	return i;
}
