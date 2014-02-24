// RUN: %check -e %s

f(int a, int b, int dir)
{
	struct
	{
		int x, y;
	} c = { };

	a & b ? c.x += dir : c.y += dir; // CHECK: error: assignment to int/if - not an lvalue
}
