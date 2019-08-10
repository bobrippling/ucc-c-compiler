// RUN: %check %s

f(){}

main()
{
	int f() __attribute__((warn_unused));
	f(); // CHECK: /warning: unused expression/
}
