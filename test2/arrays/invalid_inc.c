// RUN: %check -e %s

f(int (*x)[])
{
	return *x[1]; // CHECK: /error: dereference of pointer to incom/
}
