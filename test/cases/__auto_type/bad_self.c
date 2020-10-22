// RUN: %check -e %s

main()
{
	__auto_type x = x; // CHECK: error: undeclared identifier "x"
}
