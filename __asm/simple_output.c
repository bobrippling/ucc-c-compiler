main()
{
	int i;

	asm("movl $5, %0" : "=m"(i));
}
