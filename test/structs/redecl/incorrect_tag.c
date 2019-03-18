// RUN: %check -e %s

struct A; // CHECK: note: previous definition here
void f(union A); // CHECK: error: redeclaration of struct as union

void g(void)
{
	struct A; // CHECK: !/error/

	{
		union A // CHECK: !/error/
		{ // CHECK: !/error/
			int i; // CHECK: !/error/
		} u; // CHECK: !/error/

		u.i = 3; // CHECK: !/error/
	}
}
