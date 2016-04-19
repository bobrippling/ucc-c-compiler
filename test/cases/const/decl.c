// RUN: %check -e %s

struct Y { int i; } const a, b;

main()
{
	b.i = 2; // CHECK: error: can't modify const expression member-access
}
