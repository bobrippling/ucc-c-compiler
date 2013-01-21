main()
{
	typedef int i_td;
	typedef typeof(i_td) b;
	i_td *a;

	q(a == (char *)0); // CHECK: /warning: comparison of distinct pointer types lacks a cast/

	a = (short *)5; // CHECK: /warning: assignment type mismatch: i_td (aka 'int') \* <-- short \*/
}
