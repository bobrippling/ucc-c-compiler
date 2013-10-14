struct A
{
	int ty;
	union
	{
		struct vreg
		{
			int idx, ty;
		} r;
		struct
		{
			struct vreg r;
			int off;
		} a;
	} bits;
};

f(struct A *a)
{
	a->bits.r = a->bits.a.r;
}
