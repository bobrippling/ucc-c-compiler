// RUN: %check -e %s
int x[];

f()
{
	return sizeof(x); // CHECK: /error: sizeof incomplete type/
}
