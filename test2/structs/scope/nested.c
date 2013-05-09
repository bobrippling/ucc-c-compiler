// RUN: %ucc -c %s

main()
{
	struct A { int j; } *p;
	struct B { void *p; };
	f(p->j);

	{
		struct A { int i; } *p; // new type
		struct B b;             // use old type

		f(p->i, b.p);
	}
}
