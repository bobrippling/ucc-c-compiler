struct A
{
	int sorry_am_i_in_your_way;
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
	struct A a, *ap = &a;

	ap->bp = &a.b;

	return a.bp == &ap->b ? 0 : 1;
}
