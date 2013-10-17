// RUN: %check -e %s
f()
{
	static int ar2[]; // CHECK: /error: array has an incomplete size/
}
