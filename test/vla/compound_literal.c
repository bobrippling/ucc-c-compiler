// RUN: %check -e %s

f(int x)
{
	int *p = (int [x]){ 1, 2, 3 }[0]; // CHECK: error: cannot initialise variable length array
	int *q = (int [x + 1]){}; // CHECK: error: cannot initialise variable length array

	(void)p;
	(void)q;
}
