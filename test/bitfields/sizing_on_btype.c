// RUN: %ucc -fsyntax-only %s

struct A
{
	unsigned char r : 4, g : 4, /* boundary */ b : 4, a : 4;
};

_Static_assert(sizeof(struct A) == 2, "size should be fully packed");

struct B
{
	// note the int
	unsigned int r : 1, g : 1, b : 1, a : 1;
};

_Static_assert(sizeof(struct B) == sizeof(int), "size rounded to sizeof(int)");
