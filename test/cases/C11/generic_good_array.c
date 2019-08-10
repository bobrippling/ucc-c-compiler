// RUN: %ucc -fsyntax-only %s

_Static_assert(1 == _Generic(&(int[2]){0}, int(*)[4]:0, default: 1), "");
_Static_assert(0 == _Generic(&(int[2]){0}, int(*)[2]:0, int(*)[4]:1, default: 2), "");

f()
{
	int x[2];
	_Static_assert(_Generic(x, int *: 1, int[2]: 3) == 3, "");
}

g()
{
	int x[2];
	_Static_assert(_Generic((0, x), int *: 1, int[2]: 3) == 1, "");
}
