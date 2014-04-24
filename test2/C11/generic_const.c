// RUN: %ucc -fsyntax-only %s

_Static_assert(
		_Generic(2, const int: 1, default: 2) == 2,
		"int should not be compatible with const int");

const int ki;
_Static_assert(
		_Generic(ki, const int: 1, default: 2) == 2,
		"top-level quals on the ctrl-expr should be removed");

_Static_assert(
		_Generic(ki, int: 1, default: 2) == 1,
		"top-level qual removal should match here with int");

_Static_assert(
		_Generic(0, int: 1, const int: 2) == 1,
		"int should match int and const int shouldn't collide");
