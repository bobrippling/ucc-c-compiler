// RUN: %ucc -fsyntax-only %s

struct A
{
	int i, j;
};

f(p)
	struct A *p;
{
	return p->j;
}
