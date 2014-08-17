// RUN: %ucc -fsyntax-only %s

static int x[2];
_Static_assert(
	_Generic(x, int *: 1, int[2]: 2) == 1,
	"not decaying _Generic expr?");
