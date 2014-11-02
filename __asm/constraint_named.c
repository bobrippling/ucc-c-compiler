// RUN: %ocheck 3 %s

main()
{
	int i;
	__asm("mov %[src], %[dest]"
			: [dest] "=r"(i)
			: [src]  "r"(3)
			);
	return i;
}
