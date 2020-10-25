// RUN: %check -e %s

struct A
{
	int ar[3];
};

int i = 1;
int j = __builtin_offsetof(struct A, ar[i]); // CHECK: error: global scalar initialiser not constant
