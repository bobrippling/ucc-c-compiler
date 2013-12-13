// RUN: %layout_check %s

struct A
{
	int i;
} a;

struct A *p = &a;
