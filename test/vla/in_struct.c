// RUN: %check -e %s

g(int n)
{
	struct A
	{
		union
		{
			struct B
			{
				char buf[n]; // CHECK: error: member has variably modifed type 'char[vla]'
				char *p;
			} b;
		} bits;
	} a;

	a.bits.b.p = a.bits.b.buf;
}
