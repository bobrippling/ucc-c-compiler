struct A
{
	struct A *next;
};

struct A *next(struct A *p)
{
	return p->next;
}
