// RUN: %check -e %s

struct A
{
	int x;
	const char c;
};

int main()
{
	struct A a = { 1, 2 };

	a.x = 3; // CHECK: !/warn|error/
	a.c = 2; // CHECK: error: can't modify const expression struct

	struct A cpy;

	cpy = a; // CHECK: error: can't assign struct - contains const member
}
