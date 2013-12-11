struct A
{
	int i, j, k;
	int x[2];
};

main()
{
	struct A a, b, c = { 1, 2, 3, 4, 5 };

	a = b = c;

	f(&a, &b, &c);
}
