main()
{
	int i;
	i = 5;
	asm("movl %0, %%eax" : : "b" (i));
}
