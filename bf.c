struct half_bytes
{
	int f_1;
	int bf_1 : 3;
	int : 0;
	unsigned bf_2 : 7;
	int f_2;
};

pbf(struct half_bytes *p);

pbf(struct half_bytes *p)
{
	printf("sz=%d, p = %d %d %d %d\n",
			sizeof(*p),
			p->f_1, p->bf_1, p->bf_2, p->f_2);
}

main()
{
	struct half_bytes a;

	a.f_1 = -1UL;
	a.bf_1 = -1UL;
	a.bf_2 = -1UL;
	a.f_2 = -1UL;

	//a.f_1 = 1; // BUG
	//a.bf_1 = 2;
	//a.bf_2 = 3;
	//a.f_2 = 4;

	pbf(&a);
}
