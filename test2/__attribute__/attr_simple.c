// RUN: %ucc %s
// RUN: %check %s

main()
{
	int f() __attribute__((warn_unused));
	f(); // CHECK: /warning: unused expression/
}
