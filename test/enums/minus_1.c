// RUN: %ucc -fsyntax-only %s

enum E
{
	NEG_1 = -1
};

_Static_assert(NEG_1 == -1, "NEG_1 is an int - should be comparable");
