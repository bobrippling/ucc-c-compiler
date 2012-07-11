main()
{
	//struct defined { int i; };

	//1 ? (struct A *)5 : 0;

	0 ? (struct B *)0 : (int *)2; // should warn, and fold to (void *)2

	//(0 ? (void *)0 : (struct defined *)0)->i;
}
