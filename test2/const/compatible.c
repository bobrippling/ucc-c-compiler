// RUN: %ucc -fsyntax-only %s

_Static_assert(
		_Generic(2, const int: 1, default: 2) == 2,
		"int not compatible with const int");
