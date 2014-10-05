// RUN: %check -e %s

main()
{
	int a[2];
	a = 3; // CHECK: error: assignment to int[2] - not an lvalue
	// as opposed to "error: not an lvalue (cast)" from the implicit decay
}
