// RUN: %ucc -fsyntax-only %s

enum E
{
	A = -1,
	B = -2
};

_Static_assert(
		_Generic(
			A, int: 1) == 1,
		"enum mem != int");

_Static_assert(
		_Generic(
			(enum E)0,
			int: 1) == 1,
		"enum != int");
