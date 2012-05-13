main()
{
	struct A
	{
		int i;
	} x[2], *p;

	p = &x[0];

	p++->i = 2;
	p--->i = 3;

	return p->i + (++p)->i;
}
