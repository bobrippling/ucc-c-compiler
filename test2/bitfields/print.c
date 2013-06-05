// RUN: %ucc -o %t %s
// RUN: %t | %output_check 'sz=16, p = -1 -1 127 -1'

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

	pbf(&a);
}
