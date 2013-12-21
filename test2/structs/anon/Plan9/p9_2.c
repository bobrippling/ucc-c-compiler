// RUN: %ocheck 5 %s -fplan9-extensions

typedef struct B
{
	int k;
} B;

struct A
{
	int i;
	B;
};

f(struct A *p)
{
	p->k = 2;
	p->B.k += 3;
}

main()
{
	struct A p;
	f(&p);
	/* extension #2 - typedef name used for member
	 * only if not ambiguous */
	return p.B.k;
}
