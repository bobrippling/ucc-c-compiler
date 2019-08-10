// RUN: %ucc -fsyntax-only %s

enum
{
	A,
	B,
	X = A,
	Y = B + 50,
};
