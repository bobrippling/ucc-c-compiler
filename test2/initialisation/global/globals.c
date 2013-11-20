// RUN: %layout_check %s

#define offsetof(ty, m) (long)&(((ty *)0)->m)
struct A
{
	int i, j;
} a;

int off = offsetof(struct A, j);

int x[42];

int *p  =  x + 2 + 5;
int *p2 = &x + 50;

int *k = &a.j;

int *p_to_array = x + 3;
char *c_to_array = 2 + (char *)x + 5;

char *k_str_plus = "hi" + 2;
char *k_str      = "hello";

char *str_cast = (char *)("hi" + 2);

int i = 3;

//int i = *&x;
//int inf = 2 || 1 / 0; // what are you doing?
//int j = *&(int){5}; // gcc extension
