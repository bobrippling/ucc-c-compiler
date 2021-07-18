// RUN: %layout_check %s

struct A
{
	int : 0;
	int a;
	int : 0;
	int : 0;
	int : 0;
	int : 0;
	int b;
	int : 0;
};

struct A a = { 1, 2 };
