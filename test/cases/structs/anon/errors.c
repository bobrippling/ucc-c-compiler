// RUN: %check --only -e %s -fplan9-extensions

struct A
{
	int a_sub;
};
typedef struct A A;

struct ambig_C11 // CHECK: error: struct ambig_C11 contains duplicate member "a_sub"
{
	struct {
		int a_sub;
	}; // CHECK: /warning: tagged struct 'struct <anon struct @ .*>' is a Microsoft/Plan 9 extension/
	int a_sub;
};

struct ambig_plan9_1 // CHECK: error: struct ambig_plan9_1 contains duplicate member "a_sub"
{
	struct A; // CHECK: warning: tagged struct 'struct A' is a Microsoft/Plan 9 extension
	int a_sub;
};

struct ambig_plan9_2 // CHECK: error: struct ambig_plan9_2 contains duplicate member "a_sub"
{
	A; // CHECK: warning: tagged struct 'A {aka 'struct A'}' is a Microsoft/Plan 9 extension
	int a_sub;
};

struct bad_before // CHECK: error: struct bad_before contains duplicate member "a"
{
	int a;
	struct
	{
		int b, a;
	}; // CHECK: /warning: tagged struct 'struct <anon struct @ .*>' is a Microsoft/Plan 9 extension/
};

struct bad_after // CHECK: error: struct bad_after contains duplicate member "a"
{
	struct
	{
		int b, a;
	}; // CHECK: /warning: tagged struct 'struct <anon struct @ .*>' is a Microsoft/Plan 9 extension/
	int a;
};
