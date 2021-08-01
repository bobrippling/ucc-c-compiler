// RUN: %check --only -e %s
extern int (*f)(struct A *); // CHECK: warning: declaration of 'struct A' only visible inside function

main()
{
	struct A a; // CHECK: error: "a" has incomplete type 'struct A'
	f(&a); // CHECK: /warning: mismatching types, argument/
}
