// RUN: %check -e %s

f(struct A { int i; } *p)
{
	// should be able to initialise this - decl should be picked up
	struct A a = { 1 }; // CHECK: !/error/
	*p = a;
}

main()
{
	struct A a; // CHECK: error: "a" has incomplete type 'struct A'
	f(&a);
}
