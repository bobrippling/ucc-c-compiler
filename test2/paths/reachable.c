// RUN: %check %s

f(i) // CHECK: /warning: control reaches end of non-void function f/
{
	if(i)
		return 3;

	// bang
}
