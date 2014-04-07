f(int n)
{
	int x[n][n];
}

g(int n)
{
	struct A
	{
		union
		{
			struct B
			{
				char buf[n];
				char *p;
			} b;
		} bits;
	} a;

	a.bits.b.p = a.bits.b.buf;
}
