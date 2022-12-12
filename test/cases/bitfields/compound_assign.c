// RUN: %ocheck 5 %s

struct A
{
	int x : 16;
};

f(struct A *p)
{
	p->x += 4;
}

main()
{
#include "../ocheck-init.c"
	struct A a;
	a.x = 1;
	f(&a);
	return a.x;
}
