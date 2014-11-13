// RUN: %check -e %s

struct A
{
	int i;
} a = { 1 };

f()
{
	struct A b = a; // CHECK: !/error/
	struct A c = { a }; // CHECK: error: mismatching types, initialisation:
}
