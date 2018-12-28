// RUN: %check %s
int (*p)();
main()
{
	p + 1; // CHECK: /warning: arithmetic on function pointer/
	p[0]; // CHECK: /warning: arithmetic on function pointer/
}
