// RUN: %check --only %s

void q(int x){}

main()
{
	typedef int i_td;
	typedef __typeof(i_td) b;
	i_td *a = 0;

	q(a == (char *)0); // CHECK: warning: distinct pointer types in comparison lacks a cast

	a = (short *)5; // CHECK: warning: mismatching types, assignment
}
