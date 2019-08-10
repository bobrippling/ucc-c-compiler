// RUN: %layout_check %s

struct B
{
	int a;
	struct C
	{
		int j, k;
	} st;
	int b;
} def = {
	4,
	.st = 2,
	5 // def.st.k, not def.b
};
