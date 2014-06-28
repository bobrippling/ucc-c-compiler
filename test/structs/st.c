struct A
{
	int pad;
	struct B
	{
		int b_1;
		struct C
		{
			int c_1, c_2;
		} c, *cp;
		int b_2;
	} b, *bp;
	int a_1;
};

main()
{
	struct A a, *ap;

	ap = &a;
	ap->bp = &a.b;
	ap->bp->cp = &a.b.c;

	ap->bp->b_2 = 3;

	return a.b.b_2 == 3 ? 0 : 1;
}
