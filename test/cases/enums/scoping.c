// RUN: %ucc -fsyntax-only %s

enum E
{
	A,
	B = A, // enum reuse
	C,
	D = sizeof(B),
};

_Static_assert(A == 0, "");
_Static_assert(B == 0, "");
_Static_assert(C == 1, "");
_Static_assert(D == sizeof(int), "");

int main()
{
	enum Nest
	{
		C = C, // enum member out-of-scope lookup
		A = D,
		X = A,
	};

	_Static_assert(C == 1, "");
	_Static_assert(A == sizeof(int), "");
	_Static_assert(X == sizeof(int), "");
}
