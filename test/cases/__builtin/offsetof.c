// RUN: %ucc -fsyntax-only %s

struct A
{
	char c; // 0-4
	struct B
	{
		int i;
	} b; // 4-8
	struct C
	{
		int j, z;
	} ar[2]; // 8-24 - [0] = 8-16, [1] = 16-24
	int i; // 24-28
};

_Static_assert(24 == __builtin_offsetof(struct A, i), "");
_Static_assert(4 == __builtin_offsetof(struct A, b.i), "");
_Static_assert(4 == __builtin_offsetof(struct { int a, b; }, b), "");
_Static_assert(20 == __builtin_offsetof(struct A, ar[1].z), "");
