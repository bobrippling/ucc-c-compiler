// RUN: %check -e %s

f(struct A { int i; } *p)
{
	// should be able to initialise this - decl should be picked up
	struct A a = { 1 }; // CHECK: !/error/
	*p = a;
}

main()
{
	struct A a; // CHECK: /error: struct A is incomplete/
	f(&a);
}
