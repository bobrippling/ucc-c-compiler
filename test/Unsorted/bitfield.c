struct A
{
	int a : 1;
	int b : 1;
};

main()
{
	struct A x;

	x.b = 5;

	return x.b;
}
