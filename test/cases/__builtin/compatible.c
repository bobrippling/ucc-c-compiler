// RUN: %ucc -fsyntax-only %s

_Static_assert(
		!__builtin_types_compatible_p(enum { f, b }, enum { h, d }),
		"");
