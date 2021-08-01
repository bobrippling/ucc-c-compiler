// RUN: %ucc -fsyntax-only %s

struct A
{
	int : 10;
	int : 0;
	int a;
	int : 0;
	int b;
};

struct A a;

_Static_assert(sizeof(a) == 12, "int:0 member shouldn't affect size");
