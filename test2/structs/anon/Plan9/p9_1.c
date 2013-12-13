// RUN: %ocheck 4 %s -fplan9-extensions
struct A
{
	int a, b;
};
f(struct A *p)
{
	p->a = 1;
	p->b = 2;
}

main()
{
	struct B
	{
		int k;
		struct A;
	} b;

	struct A *p;

	/* extension #1 - auto conversion
	 * assignments and arguments */
	p = &b;
	f(&b);

	b.b = 3;

	return p->b + b.a;
}
