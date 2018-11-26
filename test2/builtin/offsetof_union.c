// RUN: %ucc -fsyntax-only %s

union x
{
	int a, b, c;
};

_Static_assert(__builtin_offsetof(union x, a) == 0, "");
_Static_assert(__builtin_offsetof(union x, b) == 0, "");
_Static_assert(__builtin_offsetof(union x, c) == 0, "");
