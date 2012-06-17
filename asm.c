main()
{
	int i = 5;
	asm("movl %%eax, %0" : : "" (i));
}
