// RUN: %check -e %s

main()
{
	int a[2];
	a = 3; // CHECK: /error: assignment to.*identifier/
	// as opposed to "error: not an lvalue (cast)" from the implicit decay
}
