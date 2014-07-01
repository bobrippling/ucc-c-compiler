// RUN: %check %s

struct A // CHECK: /warning: struct with flex-array embedded/
{
	int n;
	int p[];
} yo[2];
