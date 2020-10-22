// RUN: %check %s -pedantic
struct A
{
	int x[]; // CHECK: /warning: struct with just a flex-array is an extension/
};
