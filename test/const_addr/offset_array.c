// RUN: %ucc -fsyntax-only %s

struct A
{
	char c;
	struct B
	{
		int i;
	} b;
	struct C
	{
		int j, z;
	} ar[2];
	int i;

	int multi[2][5][3];
};

_Static_assert(
		&((struct A *)0)->ar[1] == 16,
		"");

_Static_assert(
		(unsigned long)&((struct A *)0)->multi[1][3][2] == 132,
		"");
