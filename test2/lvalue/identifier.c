// RUN: %check -e %s

main()
{
	int x[2];

	x = 3; // CHECK: error: assignment to int[2]/identifier - not an lvalue
}
