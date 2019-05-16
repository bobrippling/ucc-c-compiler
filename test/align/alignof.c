// RUN: %ucc -fsyntax-only %s

_Static_assert(
		__alignof(char *) + _Alignof(int) + __alignof__ 5 * 2
		==
		20,
		"");
