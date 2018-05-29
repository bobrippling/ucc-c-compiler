// RUN: %check --prefix=default %s -Wmismatch-ptr
// RUN: %check --prefix=explicit %s -Wno-mismatch-ptr -Wmismatch-ptr-explicit

int f();

main()
{
	int (*pf)();
	void *p = &f; // CHECK-default: /warning: implicit cast from function-pointer to pointer/
	// CHECK-explicit: ^!/warning: implicit cast from function-pointer to pointer/

	void *q = (void *)&f; // CHECK-default: !/warning:.*cast/
	// CHECK-explicit: ^/warning: cast from function-pointer to pointer/

	pf = f; // CHECK-default: !/warn/
	// CHECK-explicit: ^!/warn/

	p = f; // CHECK-default: /warning: implicit cast from function-pointer to pointer/
	// CHECK-explicit: ^!/warning: implicit cast from function-pointer to pointer/

	p = pf; // CHECK-default: /warning: implicit cast from function-pointer to pointer/
	// CHECK-explicit: ^!/warning: implicit cast from function-pointer to pointer/

	pf = p; // CHECK-default: /warning: implicit cast from pointer to function-pointer/
	// CHECK-explicit: ^!/warning: implicit cast from pointer to function-pointer/

	pf = (int *)5; // CHECK-default: /warning: implicit cast from pointer to function-pointer/

	pf = f; // CHECK-default: !/warn/
	// CHECK-explicit: ^!/warn/

	p = (void *)2; // CHECK-default: !/warning:.*cast/
	// CHECK-explicit: !/warning:.*cast/
}
