// RUN: %ocheck 1 %s

struct half_bytes
{
	int f_1;
	int bf_1 : 3;
	unsigned bf_2 : 7;
	int f_2;
};

read_bf(struct half_bytes *p)
{
	return p->bf_2;
}

pbf(struct half_bytes *p)
{
	printf("%d %d %d %d\n",
		p->f_1, p->bf_1, p->bf_2, p->f_2);
}

main()
{
	struct half_bytes a;

	memset(&a, 0, sizeof a);

	a.f_1 = 1;
	a.bf_1 = 2;
	a.bf_2 = 1;
	a.f_2 = 4;

	pbf(&a);

	int i = read_bf(&a);

	//printf("read as %d\n", i);
	return i;
}
