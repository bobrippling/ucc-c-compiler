main()
{
	long l;
	int i;
	short s;
	char c;

	__asm("movq $5, %0" : "=r"(l));
	__asm("movl $5, %0" : "=r"(i));
	__asm("movw $5, %0" : "=r"(s));
	__asm("movb $5, %0" : "=r"(c));
}
