// RUN: %ucc %s -c 2>&1 | %check %s

f(i) // CHECK: /warning: control reaches end of non-void function f/
{
	if(i)
		return 3;

	// bang
}
