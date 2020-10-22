// RUN: %check %s

int f();

main()
{
	1, f(); // CHECK: /warning: left hand side of comma is unused/
}
