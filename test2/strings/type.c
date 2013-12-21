// RUN: %ucc -fsyntax-only %s

_Static_assert(
		_Generic(((void)0, "hello"), char *: 1) == 1,
		"string literal not char *");
