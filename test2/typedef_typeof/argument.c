// RUN: %ucc -fsyntax-only %s

struct A
{
	struct
	{
		int x, y;
	} *pt;
};

f(typeof(*((struct A *)0)->pt) *pt)
{
	return pt->y;
}
