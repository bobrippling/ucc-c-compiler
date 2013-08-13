// RUN: %layout_check %s

struct A
{
	int : 10;
	int : 0;
	int a;
	int : 0;
	int b;
};

struct A a = { 1, 2 };
