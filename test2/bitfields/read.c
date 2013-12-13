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

main()
{
	struct half_bytes a;


	a.f_1 = 1;
	a.bf_1 = 2;
	a.bf_2 = 1;
	a.f_2 = 4;

	int i = read_bf(&a);
	//printf("read as %d\n", i);
	return i;
}
