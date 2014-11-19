main()
{
	int a, b;

	asm("movl $5, %eax");

	b = 2;

	asm("movl %%eax, %0; xorl %%eax, %%eax" : "=r"(a) : ""(b) : "%eax");

	return a;
}
