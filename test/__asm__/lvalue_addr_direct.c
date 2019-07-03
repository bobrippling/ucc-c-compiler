int printf(const char *, ...);

main()
{
	int i;
	int *p;

	// "=m"(i) - i written but also addressed
	__asm(
			"movl $5, %1\n\t"
			"lea %1, %0"
			: "=r"(p), "=m"(i)
			);

	// "=m"(i) should mean that because 'i' is an lvalue we get its address directly
	if(p != &i)
		abort();

	return 0;
}
