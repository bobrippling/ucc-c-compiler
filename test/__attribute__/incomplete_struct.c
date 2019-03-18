// RUN: %check -e %s

struct A
{
	int x;
};

struct B
{
	int x;
} __attribute((aligned(sizeof(struct A)))); // CHECK: !/error/

struct C
{
	int x;
} __attribute((aligned(sizeof(struct C)))); // CHECK: error: sizeof incomplete type struct C
