// RUN: %ucc -g %s -o %t

struct A
{
	int i;
	struct A *next;
};

struct A a = {
	.next = &a,
	.i = 3,
};
