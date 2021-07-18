// RUN: %ucc -fsyntax-only %s

struct A
{
	int i;
};

f(struct A *const p, const struct A *q)
{
	// ensure the const on 'p' doesn't affect p->i
	return p->i++ + q->i;
}
