// RUN: %check %s
f()
{
}

main()
{
	f();  // CHECK:!/warning: too many arguments/
	g();  // CHECK:!/warning: too many arguments/
	g(2); // CHECK:!/warning: too many arguments/
	f(1); // CHECK: /warning: too many arguments/
}
