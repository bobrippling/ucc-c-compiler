// RUN: %ucc -fsyntax-only %s

_Static_assert(
		0x1FFFFFFFFFFFFFFF == (unsigned long)-1 / 8,
		"bad cast parsing");
