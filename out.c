main()
{
	int i;
	asm("movl $5, %0" : "=r"(i));
	return i;
}
