// RUN: %check -e %s

f(int n)
{
	int x[n] = { 1, 2, 3 }; // CHECK: error: cannot initialise variable length array
}
