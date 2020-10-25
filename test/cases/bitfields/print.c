// RUN: %ocheck 0 %s

struct half_bytes
{
	int f_1;
	int bf_1 : 3;
	int : 0;
	unsigned bf_2 : 7;
	int f_2;
};

chk_bf(struct half_bytes *p)
{
	_Static_assert(sizeof(*p) == 16, "");
	void abort(void);
	if(p->f_1 != -1)
		abort();
	if(p->bf_1 != -1)
		abort();
	if(p->bf_2 != 127)
		abort();
	if(p->f_2 != -1)
		abort();
	return 0;
}

main()
{
	struct half_bytes a;

	a.f_1 = -1UL;
	a.bf_1 = -1UL;
	a.bf_2 = -1UL;
	a.f_2 = -1UL;

	chk_bf(&a);
	return 0;
}
