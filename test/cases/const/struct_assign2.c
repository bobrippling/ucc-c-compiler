// RUN: %check -e %s

struct A
{
	int x;
	const char c;
};

struct Y { int i; } const y1, y2;

int main()
{
	struct A a = { 1, 2 };

	a.x = 3; // CHECK: !/warn|error/
	a.c = 2; // CHECK: error: can't modify const expression member-access

	struct A cpy;

	cpy = a; // CHECK: error: can't assign struct - contains const member

	y2.i = 2; // CHECK: error: can't modify const expression member-access
}
