// RUN: %check %s

int *f()
{
	static int a[2];

	return a + 3; // CHECK: /index 3 out of bounds/
}
