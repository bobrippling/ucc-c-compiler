// RUN: %ucc -fsyntax-only %s

g(int, int);

f()
{
	// both x and y are 8-byte aligned
	_Alignas(8) int x = 1, y = 2;

	_Static_assert(_Alignof(x) == 8, "");
	_Static_assert(_Alignof(y) == 8, "");

	g(x, y);
}
