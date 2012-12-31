#define offsetof(ty, m) (long)&(((ty *)0)->m)
struct A
{
	int i, j;
} a;

int off = offsetof(struct A, j);

int x[42];

int *p  =  x + 2 + 5;
int *p2 = &x + 50;

//int i = &*x;

int j = *&(int){5};

int *k = &a.j;

int inf = 2 || 1 / 0;

char *str_plus = "hello" + 5;
