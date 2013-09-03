// RUN: %check -e %s

f(struct A *p);

g(struct A *p)
{
	f(p); // CHECK: /warning: mismatching argument/
}

main()
{
	struct A yo; // CHECK: /error: struct A is incomplete/
	// CHECK: ^ /note: forward declared here/
	f(&yo); // CHECK: /warning: mismatching argument/
	g(&yo); // CHECK: /warning: mismatching argument/
}
