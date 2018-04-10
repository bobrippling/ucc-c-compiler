// RUN: %ucc -fplan9-extensions %s -o %t
// RUN: %t
struct C99
{
	union
	{
		int i;
		char c;
	};
	int k;
};

plan_9();

main()
{
	struct C99 a = {
		.k = 1,
		.i = 3,
	}; // { { 3 }, 1 }

	a.c = 2; // { { 2 }, 1 }

	if(a.i != 2 || a.k != 1)
		abort();

	plan_9();

	return 0;
}

/* extensions */

struct A
{
	int a_sub;
};

typedef struct A A;

typedef union
{
	int i;
	char c;
} U;



struct B_A { int b_a; };
struct B
{
	struct B_A;              // MS-extensions, since struct has a tag
	struct B_B { int abc; }; // MS-extensions, since def has a tag
	int i;
};

struct C
{
	A a; // Plan 9/MS-extensions, since typedef
	int i;
};

struct D
{
	int type;
	U;
};


void f(U *a)
{
}

plan_9()
{
	struct D *d = (void *)0;
	f(d); // converts to U *

	typedef struct { int k; } mem_alias;
	struct
	{
		int hello;
		mem_alias;
	} st;

	return st.mem_alias.k;
}
