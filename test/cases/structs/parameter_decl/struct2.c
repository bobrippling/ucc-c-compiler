// RUN: %check --only -e %s

f(struct A *p); // CHECK: warning: declaration of 'struct A' only visible inside function

g(struct A *p) // CHECK: warning: declaration of 'struct A' only visible inside function
{
	f(p); // CHECK: /warning: mismatching types, argument/
	return 3;
}

main()
{
	struct A yo; // CHECK: error: "yo" has incomplete type 'struct A'
	// CHECK: ^ /note: forward declared here/
	f(&yo); // CHECK: /warning: mismatching types, argument/
	g(&yo); // CHECK: /warning: mismatching types, argument/
}
