// RUN: %ocheck 0 %s

struct A
{
	int i, j, k;
	int x[2];
};

chk(struct A *p)
{
	if(p->i != 1)
		abort();
	if(p->j != 2)
		abort();
	if(p->k != 3)
		abort();
	if(p->x[0] != 4)
		abort();
	if(p->x[1] != 5)
		abort();
}

main()
{
	struct A a, b, c, d, e = { 1, 2, 3, 4, 5 };

	a = b = c = d = e;

	chk(&a);
	chk(&b);
	chk(&c);
	chk(&d);
	chk(&e);

	return 0;
}
