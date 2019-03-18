// RUN: %check -e %s -fplan9-extensions

struct A
{
	int a_sub;
};
typedef struct A A;

struct ambig_C11
{
	struct {
		int a_sub;
	};
	int a_sub; // CHECK: error: duplicate member a_sub
};

struct ambig_plan9_1
{
	struct A;
	int a_sub; // CHECK: error: duplicate member a_sub
};

struct ambig_plan9_2
{
	A;
	int a_sub; // CHECK: error: duplicate member a_sub
};

struct bad_before
{
	int a;
	struct
	{
		int b, a; // CHECK: error: duplicate member a
	};
};

struct bad_after
{
	struct
	{
		int b, a;
	};
	int a; // CHECK: error: duplicate member a
};
