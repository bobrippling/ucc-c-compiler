// RUN: %check %s

struct A
{
	int n;
	int p[];
} yo[2]; // CHECK: /warning: struct with flex-array embedded/
