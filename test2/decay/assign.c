// RUN: %check %s

main()
{
	int a[2];
	a = 3; // CHECK: /error: not an lvalue (identifier)/
	// as opposed to /error: not an lvalue (cast)/ from the implicit decay
}
