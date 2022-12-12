// RUN: %check --only %s
int f();

main()
{
	int (*pf)();
	void *p = &f; // CHECK: warning: implicit cast from function-pointer to pointer

	pf = f;
	p = f; // CHECK: warning: implicit cast from function-pointer to pointer

	p = pf; // CHECK: warning: implicit cast from function-pointer to pointer

	pf = p; // CHECK: warning: implicit cast from pointer to function-pointer
	pf = (int *)5; // CHECK: warning: implicit cast from pointer to function-pointer
	// CHECK: ^/mismatching.*assign/

	pf = f;
	p = (void *)2;
}
