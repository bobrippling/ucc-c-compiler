int main()
{
	struct A
	{
		struct B
		{
			int a, b;
		} b;
	} a, *pa = &a;


#ifdef USC_REPR
	a.b.a = 5;
	a.b.b = 1;

	(&(&a)->b)->a = 1;
#else
	//a.b.b = 1;

	(*(&(*pa).b)).a = 2;
#endif

	return a.b.a + a.b.b;
}
