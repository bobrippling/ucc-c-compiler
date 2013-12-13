// RUN: %ocheck 0 %s
struct A
{
	int f_1;
	int bf_1 : 3;
	int f_2;
	int bf_2 : 7;
	int bf_3 : 10;
};

write_bf(struct A *p)
{
	p->bf_2 = 35;
}

chk_bf(struct A *p, int a, int b, int c, int d, int e)
{
	if(p->f_1 != a)
		abort();
	if(p->bf_1 != b)
		abort();
	if(p->f_2 != c)
		abort();
	if(p->bf_2 != d)
		abort();
	if(p->bf_3 != e)
		abort();
	return 0;
}

main()
{
	struct A a;


	a.f_1 = 1;
	a.bf_1 = 2;
	a.f_2 = 3;
	a.bf_2 = 4;
	a.bf_3 = 5;

	chk_bf(&a, 1, 2, 3, 4, 5);

	write_bf(&a);

	chk_bf(&a, 1, 2, 3, 35, 5);

	return 0;
}
