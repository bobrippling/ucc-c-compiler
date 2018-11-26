// RUN: %check %s -pedantic -ffold-const-vlas -std=c89

main()
{
	char x[(1, 2)]; // CHECK: /warning: comma-expr is a non-standard constant expression/

	return sizeof(x);
}
