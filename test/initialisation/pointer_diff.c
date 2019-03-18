// RUN: %ucc -fsyntax-only %s

struct
{
	char a, f[3];
	int b;
	char q;
} s;

_Static_assert(s.f - &s.a == 1, "");
_Static_assert((int*)&s.q - &s.b == 1, "");
_Static_assert(&s.q - (char*)&s.b == sizeof(s.b), "");
