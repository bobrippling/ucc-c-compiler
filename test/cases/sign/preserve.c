// RUN: %ucc -fsyntax-only %s

_Static_assert(
		_Generic(-1u, unsigned: 1) == 1,
		"unsigned not preserved");

_Static_assert(
		_Generic(1 + 1u, unsigned: 1) == 1,
		"unsigned not converted to");

_Static_assert(
		_Generic(1u - 10, unsigned: 1) == 1,
		"unsigned not converted to");

_Static_assert(
		_Generic(1 - 10u, unsigned: 1) == 1,
		"unsigned not converted to");
