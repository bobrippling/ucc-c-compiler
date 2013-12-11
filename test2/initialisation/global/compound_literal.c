// RUN: %layout_check %s

int i = (int)&((struct A { int x, a; } *)0)->a;

int *x = &i;
int *y = &i + 2;

//int cl = (int){2}; not constant
int *cp = &(int){2};

int (*cpa)[] = &(int[]){2, 3};
