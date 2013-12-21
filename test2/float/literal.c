// RUN: %ucc -fsyntax-only %s

_Static_assert(
		__builtin_types_compatible_p(
			typeof(5.02f),
			float), "?");

_Static_assert(
		__builtin_types_compatible_p(
			typeof(5.02),
			double), "?");

_Static_assert(
		__builtin_types_compatible_p(
			typeof(5.02L),
			long double), "?");
