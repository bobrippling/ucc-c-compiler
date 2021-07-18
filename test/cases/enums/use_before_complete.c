// RUN: %ucc -fsyntax-only %s

typedef enum E
{
	A = 0,
	B,

	C = A + 10,
} E;
