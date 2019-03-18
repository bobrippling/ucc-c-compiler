// RUN: %ucc %s
// RUN: %check %s

main()
{
	return (char *)0 == (int *)5; // CHECK: /warning: mismatching types, comparison lacks a cast/
}
