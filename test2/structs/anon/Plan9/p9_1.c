struct A
{
	int a, b;
};
f(struct A *);

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

	b.b = 2;

	return p->b;
}
