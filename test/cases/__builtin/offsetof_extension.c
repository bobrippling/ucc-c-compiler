// RUN: %check %s

struct A
{
	struct B
	{
		int x, y;
	} b;
	struct
	{
		int z;
	};
};

_Static_assert(
		__builtin_offsetof(struct A, b.x) == 0, // CHECK: warning: extended designator in offsetof()
		"");

_Static_assert(
		__builtin_offsetof(struct A, z), // CHECK: !/warning: extended designator/
		"");
