// RUN: %ucc -fsyntax-only %s

_Static_assert(
		_Generic("abc", char[4]: 20) == 20,
		"");

