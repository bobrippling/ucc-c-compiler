// RUN: %ocheck 0 %s

a(){}
b(){}
c(){}
d(){}

f()
{
	return 3;
}

void check_printf(const char *a, int x, double y)
{
	if(x != 3){
		_Noreturn void abort();
		abort();
	}
	if(y != 1){
		_Noreturn void abort();
		abort();
	}
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
	check_printf("%d %.1f\n", f(a(), b(), c(), d(), i), 1.0);

	return 0;
}
