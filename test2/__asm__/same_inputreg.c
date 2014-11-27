// RUN: %check -e %s

main()
{
	__asm("%0 %1"
			:
			: "c"(3), "c"(5)); // CHECK: error: input constraint unsatisfiable
	// 'c' has to be 3 and 5
}
