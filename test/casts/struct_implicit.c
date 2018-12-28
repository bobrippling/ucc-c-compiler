// RUN: %check %s

struct A
{
	int i;
} a;

int *p = &a.i; // CHECK: !/warn/
