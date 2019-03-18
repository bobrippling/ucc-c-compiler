// RUN: %ucc -fsyntax-only %s

struct A
{
	int x;
	int ar[1];
};

_Static_assert(
		__builtin_offsetof(struct A, ar) == sizeof(int),
		"");
