// RUN: %ucc -fsyntax-only %s

struct A
{
	unsigned x : 2;
} a;

__typeof(a.x + 2) i;
int i;

_Static_assert(
		__builtin_types_compatible_p(
			__typeof(a.x + 2),
			int),
		"wrong bitfield integer promotion");

_Bool b;          // promoted to signed int
unsigned char c;  // promoted to signed int
unsigned short s; // promoted to signed int

__typeof(b + 2) i;
__typeof(c + 2) i;
__typeof(s + 2) i;
