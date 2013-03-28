// RUN: %ucc -c %s
struct A
{
	char a, b, c;
} __attribute((aligned(8))) x;

_Static_assert(_Alignof(x) == 8, "");
