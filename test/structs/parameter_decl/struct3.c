// RUN: %check -e %s
extern int (*f)(struct A *);

main()
{
	struct A a; // CHECK: error: "a" has incomplete type 'struct A'
	f(&a); // CHECK: /warning: mismatching types, argument/
}
