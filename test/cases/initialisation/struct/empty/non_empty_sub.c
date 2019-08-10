// RUN: %check %s -std=c11

struct A
{
	struct
	{
		int i;
	};
}; // CHECK: !/warning: struct has no named members/

struct A a = { .i = 5 }; // CHECK: !/warning: missing {} initialiser for empty struct/
