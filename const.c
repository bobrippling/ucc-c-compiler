int x[2];

#define offsetof(t, m) (unsigned long) & ((t *)0)->m

struct A
{
	int i, j, k;
};

int *p = 5 + x + 3 + 2 + offsetof(struct A, j);

int a[x - x + 1];
