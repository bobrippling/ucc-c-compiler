// RUN: %check %s

int *f()
{
	static int a[2];

	return a + 2; // CHECK: /index 2 out of bounds/
}
