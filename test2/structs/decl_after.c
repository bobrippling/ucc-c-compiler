struct A;

f(struct A *p);

struct A
{
	int i;
};

f(struct A *p)
{
	return p->i;
}
