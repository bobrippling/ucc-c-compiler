// RUN: %check -e %s
extern int (*f)(struct A *);

main()
{
	struct A a; // CHECK: /error: struct A is incomplete/
	f(&a); // CHECK: /warning: mismatching types, argument/
}
