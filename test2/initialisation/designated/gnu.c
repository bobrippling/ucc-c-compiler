// RUN: %layout_check %s
// RUN: %check %s

struct A
{
	int a, b;
} x = {
	a: 1, // CHECK:  warning: use of old-style GNU designator
	b: 3  // CHECK: warning: use of old-style GNU designator
};

int ints[] = {
	[3] 2 // warning: use of GNU 'missing =' designator
};

/*
struct B
{
	struct {
		int q;
		struct A a;
	} n;
} y = {
	n.a.b: 3
};
*/
