int a, b;

main()
{
	// swap
	__asm("xchg %1, %0" : "=m"(a) : "r"(b));

#ifdef MORE
	// a = b; -- reg2mem
	__asm("mov %1, %0" : "=m"(a) : "r"(b));

	// a = b; -- reg2reg
	__asm("mov %1, %0" : "=r"(a) : "r"(b));

	// a = b; -- mem2reg
	__asm("mov %1, %0" : "=r"(a) : "m"(b));
#endif
}
