// RUN: %check -e %s
f()
{
	struct B a[] = { 1 }; // CHECK: error: initialising incomplete type 'struct B[]'
}
