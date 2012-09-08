f(int *p)
{
	asm("movl $5, %0" : "=m"(*p));
}
