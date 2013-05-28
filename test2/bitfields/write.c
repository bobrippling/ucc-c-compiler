// RUN: %output_check %s '1 2 3 4 5' '1 2 3 35 5'
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

pbf(struct A *p)
{
	printf("%d %d %d %d %d\n",
		p->f_1, p->bf_1, p->f_2, p->bf_2, p->bf_3);
}

main()
{
	struct A a;

	a.f_1 = 1;
	a.bf_1 = 2;
	a.f_2 = 3;
	a.bf_2 = 4;
	a.bf_3 = 5;

	pbf(&a);

	write_bf(&a);

	pbf(&a);
}
