// RUN: %check %s
struct A
{
	int x[]; // CHECK: /warning: struct with just a flex-array is an extension/
};
