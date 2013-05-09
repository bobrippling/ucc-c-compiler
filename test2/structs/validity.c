// RUN: %ucc %s

struct A
{
	int i;
};

main()
{
	struct A a, *p;

	p->i;
	a.i;
}
