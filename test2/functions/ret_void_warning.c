// RUN: %check %s
// RUN: %check --prefix=wall %s -Wreturn-void

void g();

void f()
{
	return g(); // CHECK: !/warn/
	// CHECK-wall: ^ warning: void function returns void expression
}

void h()
{
	return ^{ // CHECK: !/warn/
	// CHECK-wall: ^ warning: void function returns void expression

		return g(); // CHECK: !/warn/
		// CHECK-wall: ^ warning: void function returns void expression
	}();
}
