// RUN: %ucc -fsyntax-only %s

_Static_assert(
		_Generic('a', int: 5, char: 2)
		==
		5,
		"");
