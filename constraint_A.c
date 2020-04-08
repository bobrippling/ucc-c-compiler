f()
{
	char c = 3;
	__asm("got %0\n\tand %1" : : "A"(&c), "A"(c));
}
