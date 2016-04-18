// RUN: %check -e %s

struct A
{
	int x : sizeof(struct A); // CHECK: error: sizeof incomplete type struct A
};
