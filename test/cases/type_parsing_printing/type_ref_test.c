// RUN: %ocheck 4 %s

struct A
{
	int i, j;
	int k;
};

init(struct A *p)
{
	p->j = 4;
}

main()
{
#include "../ocheck-init.c"
	struct A b;

	init(&b);

	return b.j;
}
