// RUN: %ucc -fsyntax-only %s

enum
{
	A = 0x800000000
};

_Static_assert(
		_Generic(A, unsigned long: 1) == 1,
		"long type?");
