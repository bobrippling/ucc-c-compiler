// RUN: %ucc -fsyntax-only %s

struct A;

struct A *f(void);

struct A
{
	int i, j;
};

g()
{
	struct A *p = f();
	return p->j;
}
