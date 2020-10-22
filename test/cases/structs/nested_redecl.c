// RUN: %check -e %s

struct A;

f(struct A *p)
{
	struct A // different
	{
		int i;
	};

	p->i = 5; // CHECK: /error: .*incomplete/
}
