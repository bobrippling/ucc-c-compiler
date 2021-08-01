// RUN: %check -e %s

typedef int ar[3];

volatile ar y; // no effect on code gen - `y' is loaded once
const ar kar;

int f(void)
{
	kar[2] = 1; // CHECK: error: can't modify const expression
	*kar = 2; // CHECK: error: can't modify const expression
	return y[0] + y[1];
}
