// RUN: %ucc -fsyntax-only %s

struct A
{
	char i, j;
} __attribute((aligned(8)));

struct A a;

_Static_assert(_Alignof(a) == 8, "misaligned/attr not picked up");
