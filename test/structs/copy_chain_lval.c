// RUN: %check -e %s
main()
{
	struct A
	{
		int i;
	} a = { 1 }, b = { 2 }, c = { 3 };

	(a = b) = c; // CHECK: error: assignment to struct A - not an lvalue
}
