main()
{
	struct defined { int i; };

	x(1 ? (struct A *)5 : 0);

	struct B { int i; };

	//__typeof(*(0 ? (struct B *)0 : (int *)2)) a; // should warn, and fold to (void *)2

	y((0 ? (void *)0 : (struct defined *)0)->i);
}
