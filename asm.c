main()
{
	int i = 5;
	asm("mov %0, eax" : : "b" (i));
}
