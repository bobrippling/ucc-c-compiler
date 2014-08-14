a(){}
b(){}
c(){}
d(){}

f()
{
	return 3;
}

/* Need sufficient nested-calls to get 3-callee save registers in action
 * Also need a call to printf with a float to execute movdqa instructions
 * Also need a single argument so the stack_alloc_n is misaligned by a pushq
 *
 * Normally this pushq is handled and realigned but the callee-save registers
 * foul this up.
 */
main(int argc)
{
	int i = 5;
	printf("%d %.1f\n", f(a(), b(), c(), d(), i), 1.0);
}
