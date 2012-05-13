struct A
{
	int i, j;
};

init(struct A *a)
{
	/*
	a[0].i = 0;
	a[0].j = 1;
	a[1].i = 2;
	a[1].j = 3;
	*/
	a->i = 0;
	a->j = 1;
	a++;
	a->i = 2;
	a->j = 3;
}

main()
{
	struct A a[2];
	struct A *p = a;

	init(a);

	return
		a[0].i +
		a[0].j +
		a[1].i +
		a[1].j;
}
