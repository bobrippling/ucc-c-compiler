// RUN: %ocheck 0 %s -fplan9-extensions -Werror -Wno-microsoft-anon-tag -Wno-missing-braces -Wno-tautological-pointer-compare
// RUN: %check --only %s -fplan9-extensions

extern int memcmp(const void *, const void *, unsigned long);
_Noreturn void abort(void);

void assert(_Bool b)
{
	if(!b)
		abort();
}

struct C99
{
	union
	{
		int i;
		char c;
	}; // CHECK: /warning: tagged struct '.*' is a Microsoft\/Plan 9 extension/
	int k;
};

struct XY_or_WX
{
	union
	{
		struct
		{
			char x, y;
		}; // CHECK: /warning: tagged struct '.*' is a Microsoft\/Plan 9 extension/
		struct
		{
			char w, h;
		}; // CHECK: /warning: tagged struct '.*' is a Microsoft\/Plan 9 extension/
	}; // CHECK: /warning: tagged struct '.*' is a Microsoft\/Plan 9 extension/
};

typedef struct { int xyz; } Untagged;
struct WithUntagged
{
	int type;
	Untagged; // CHECK: /warning: tagged struct 'Untagged \{aka 'struct <anon struct @ .*>'\}' is a Microsoft\/Plan 9 extension/
};

void test_c99()
{
	const struct C99 same[] = {
		{ .k = 1, .i = 3, },
		{ { 3 }, 1 },
		{ 3, 1 }, // CHECK: warning: missing braces for initialisation of sub-object 'union
		{ .i = 3, 1 },
	};

	assert(memcmp(&same[0], &same[1], sizeof(struct C99)) == 0);
	assert(memcmp(&same[1], &same[2], sizeof(struct C99)) == 0);
	assert(memcmp(&same[2], &same[3], sizeof(struct C99)) == 0);

	struct C99 copy = same[0];
	copy.c = 2; // { { 2 }, 1 }
	if(copy.i != 2 || copy.k != 1)
		abort();

	struct XY_or_WX xy_wh = { .x = 3 };
	xy_wh.x = 2;
	if(xy_wh.w != 2)
		abort();

	struct
	{
		struct B{int i;}; // CHECK: /warning: tagged struct '.*' is a Microsoft\/Plan 9 extension/
	} untag;
	untag.i = 3;
}

struct A
{
	int a_sub;
};
typedef struct A A;

/* extensions */
typedef union
{
	int i;
	char c;
} U;

struct B_A { int b_a; };
struct B
{
	struct B_A; // CHECK: /warning: tagged struct '.*' is a Microsoft\/Plan 9 extension/
	// ^MS-extension, since struct has a tag
	struct B_B { int abc; }; // CHECK: /warning: tagged struct '.*' is a Microsoft\/Plan 9 extension/
	// ^MS-extension, since def has a tag
	int i;
};

struct C
{
	A; // CHECK: /warning: tagged struct '.*' is a Microsoft\/Plan 9 extension/
	// ^Plan 9/MS-extension, since typedef
	int i;
};

struct D
{
	int type;
	U; // CHECK: /warning: tagged struct '.*' is a Microsoft\/Plan 9 extension/
	// Plan 9/MS-extension, since typedef
};

int consume_int(int x)
{
	return x;
}

int consume_u(U *a)
{
	return a->i;
}

void test_plan_9()
{
	struct D d = {
		5,
		3 // CHECK: warning: missing braces for initialisation of sub-object 'U
	};
	// converts to U *:
	assert(consume_u(&d) == 3); // CHECK: !/warn/

	typedef struct { int k; } mem_alias;
	struct
	{
		int hello;
		mem_alias; // CHECK: /warning: tagged struct 'mem_alias .*' is a Microsoft/Plan 9 extension/
	} st = {
		6,
		{ 2, },
	};

	assert(consume_int(st.mem_alias.k) == 2);
}

int main()
{
#include "../../ocheck-init.c"
	test_c99();
	test_plan_9();

	return 0;
}
