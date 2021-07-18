// RUN: %check -e %s

main()
{
	--1; // CHECK: error: compound assignment to int - not an lvalue
}
