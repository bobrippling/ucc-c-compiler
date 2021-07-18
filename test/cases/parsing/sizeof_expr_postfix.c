// RUN: %ucc -fsyntax-only %s

struct A
{
	int x;
	struct
	{
		int a;
		char q[3];
	} *y[3];
};

_Static_assert(sizeof(struct A){ 0 }.y[2]->q == 3, "");
