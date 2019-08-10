// RUN: %ucc -fsyntax-only %s

typedef int trio[3];

const trio abc; // const int [3]

_Static_assert(
		_Generic(abc, int const[3]: 1) == 1,
		"");
