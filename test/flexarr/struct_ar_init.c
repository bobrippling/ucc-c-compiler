// RUN: %check %s
struct A
{
	int n;
	int vals[];
} x[] = {
	// ucc allows this
	1, { 7 },    // CHECK: /warning: initialisation of flexible array/
	2, { 5, 6 }, // CHECK: /warning: initialisation of flexible array/
};
