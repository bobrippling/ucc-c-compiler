// RUN: %layout_check %s

typedef struct A
{
	int i, j;
} A;

A a, *b;

int *p = &a.j;
//int *q = &b->j;
