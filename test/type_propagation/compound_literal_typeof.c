// RUN: %ucc -fsyntax-only %s

_Static_assert(
		sizeof(__typeof(int[]){1,2,3})
		==
		12,
		"");
