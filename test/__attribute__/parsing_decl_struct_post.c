// RUN: %ucc -fsyntax-only %s

struct __attribute((aligned(8))) A
{
	char i, j;
};

struct A a;

_Static_assert(_Alignof(a) == 8, "misaligned/attr not picked up");
