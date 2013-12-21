// RUN: %ucc -fsyntax-only %s
_Static_assert(
		_Generic(__builtin_nanf(""),
			float: 1) == 1,
		"?");

_Static_assert(
		_Generic(__builtin_nan(""),
			double: 1) == 1,
		"?");

_Static_assert(
		_Generic(__builtin_nanl(""),
			long double: 1) == 1,
		"?");
