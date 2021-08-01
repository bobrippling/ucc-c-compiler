// RUN: %check --only -e %s

struct A f() // CHECK: error: incomplete return type
	// CHECK: ^warning: control reaches end of non-void function f
{
}

void g()
{
	f(); // CHECK: error: function call returns incomplete type 'struct A'
}
