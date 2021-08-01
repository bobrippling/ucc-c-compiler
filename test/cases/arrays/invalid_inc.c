// RUN: %check -e %s

f(int (*x)[])
{
	return *x[1]; // CHECK: /error: arithmetic on pointer to incomplete type/
}
