struct A
{
	const int k;
	int i;
};

main()
{
	const struct A x;
	struct A x2;

	//x.i = 5;
	//x.k = 5;
	x2.i = 3;
	//x2.k = 3;
}
