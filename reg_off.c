f(struct { int i, j; } *p)
{
	p->j = 3;
	return p->j;
}
