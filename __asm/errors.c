main()
{
	int i, j;

	__asm("%2" : "=r"(i) : "0"(0)); // CHECK: invalid register index %2
	__asm("%2" : "=r"(i), "=r"(j)); // CHECK: invalid register index %2
	__asm("%2" : : "r"(0), "r"(1)); // CHECK: invalid register index %2

	__asm("" : "=r"(i), "=0"(j)); // CHECK: matching constraint on output
}
