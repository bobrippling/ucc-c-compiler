// RUN: %check -e %s

main()
{
	int i = 3;

	// bad outputs
	__asm("" : ""(i)); // CHECK: error: output operand missing =/+ in constraint
	__asm("" : "&"(i)); // CHECK: error: output operand missing =/+ in constraint

	// bad inputs
	__asm("" : : ""(0));   // CHECK: error: input constraint unsatisfiable
	__asm("" : : "="(0));  // CHECK: error: input operand has =/+ in constraint
	__asm("" : : "&"(0));  // CHECK: error: constraint modifier '&' on input
	__asm("" : : "&r"(0)); // CHECK: error: constraint modifier '&' on input
}
