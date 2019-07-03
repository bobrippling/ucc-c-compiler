// RUN: %ucc -fsyntax-only %s

enum E
{
	A = 1,
	B = A + 2 // A has sizeof(int), can't be sizeof(enum E) since E isn't complete yet
};

_Static_assert(_Generic(A, int: 1) == 1, "bad enum");
_Static_assert(_Generic((enum E)0, enum E: 1, int: 2) == 1, "bad enum");
_Static_assert(_Generic((enum E)0, int: 2, default: 1) == 1, "bad enum");
