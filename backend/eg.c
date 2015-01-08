struct A
{
	int i, j;
};

f(struct A *p)
{
	p->j = 3;

	int x = g();

	p->j = 6;

	return p->i + x + p->j;
}
