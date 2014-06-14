// RUN: %debug_check %s

struct A
{
	int i;
	struct A *next;
};

struct A a = {
	.next = &a,
	.i = 3,
};
