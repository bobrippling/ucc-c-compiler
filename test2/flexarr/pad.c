// RUN: %ucc -fsyntax-only %s
struct A
{
	char c;
	// padding of 3 bytes
	int x[];
};

_Static_assert(sizeof(struct A) == 4,
		"padding not accounted for");
