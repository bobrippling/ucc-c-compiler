main()
{
	int i;
	asm("movl %1, %0" : "=r"(i) : ""(5));
	return i;
}
