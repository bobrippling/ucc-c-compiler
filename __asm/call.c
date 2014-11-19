f()
{
	asm("movl $5, %%ebx");
	return 1;
}

main()
{
	int r;
	register int q asm("ebx");

	q = 3;

	asm(
			"call f;"
			"movl %%eax, %0"
			: "=r"(r)
			: "D"(5)
			:
		 );

	return r + q;
}
