main()
{
	int a = 10, b;

	__asm__(
			"movl %1, %%eax;"
			"movl %%eax, %0;"
			: "=r"(b)        /* output */
			: "r"(a)         /* input */
			: "%eax"         /* clobbered register */
			);
}
