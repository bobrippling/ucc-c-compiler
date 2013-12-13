// RUN: %ucc %s
// RUN: %check %s

q(){}

main()
{
	typedef int i_td;
	typedef typeof(i_td) b;
	i_td *a;

	q(a == (char *)0); // CHECK: /warning: mismatching types, comparison lacks a cast/

	a = (short *)5; // CHECK: /warning: mismatching types, assignment/
}
