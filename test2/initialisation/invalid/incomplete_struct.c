// RUN: %check -e %s
f()
{
	struct B a[] = { 1 }; // CHECK: error: array has incomplete type 'struct B'
	                      // CHECK: ^error: initialising struct B[]
}
