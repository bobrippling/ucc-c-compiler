// RUN: %ucc -fsyntax-only %s

_Static_assert(
		_Generic(0LU + 0LL, unsigned long long: 1) == 1,
		"");
