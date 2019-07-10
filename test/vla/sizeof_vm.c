// RUN: %ucc -fsyntax-only %s

int f(), g();

_Static_assert(
		__builtin_constant_p(
			sizeof((short (**)[][f()]) g())),
		"sizeof vm not constant");
