struct A
{
	struct A *next;
};

struct A *f(struct A *p)
{
	// we are casting this to void and messing with the phi-logic
	return p ? p->next : 0;
}
