// RUN: %check %s

main()
{
	char x[(1, 2)]; // CHECK: /warning: comma-expr is a non-standard constant expression/

	return sizeof(x);
}
