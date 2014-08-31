main()
{
	int A, B;

	B = 3;

	__asm("mov %1, reg\n\t"
			"mov reg %0"
			: "=r"(A)
			: "r"(B)); // "A" and "B" may be the same register

	__asm("mov %1, reg\n\t"
			"mov reg %0"
			: "+r"(B)
			: "r"(A)); // "A" and "B" may be the same register

	return A;
}
