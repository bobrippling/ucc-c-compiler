main()
{
	struct X;

	struct X *a;

	{
		struct X { int i; };
		a->i = 5;
	}

	{
		struct X { int *p; };
		a->p = 2;
	}
}
