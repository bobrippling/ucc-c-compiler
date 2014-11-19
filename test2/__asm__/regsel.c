// RUN: %ocheck 10 %s
// RUN: %check -e %s -DOVER_LIMIT

main()
{
	int i = 3;

	__asm("incl %0" : "+r"(i));
	// try not to choose a callee-save reg for "r"(3)
	__asm("addl %1, %0" : "+r"(i) : "r"(3));

	// run out of non-callee-save regs:
	__asm("addl %1, %0"
			: "+r"(i)
			: "r"(3), "r"(3), "r"(3), "r"(3), "r"(3), "r"(3), "r"(3)
			, "r"(3), "r"(3), "r"(3), "r"(3), "r"(3), "r"(3)
#ifdef OVER_LIMIT
			, "r"(3) // CHECK: error: input constraint unsatisfiable
#endif
			);

	return i;
}
