// RUN: %ocheck 5 %s
main()
{
	struct A
	{
		int i;
	} x[2], *p;

	p = &x[0];

	p++->i = 2;
	p--->i = 3;

	// x = { { 2, 3 } }

	return p->i + (p + 1)->i;
}
