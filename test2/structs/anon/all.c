// RUN: %ucc %s
struct C99
{
	union
	{
		int i;
		char c;
	};
	int k;
};

struct ambig
{
	A;
	int a_sub;
};

main()
{
	struct C99 a = { /* designated init isn't C99 */
		.k = 1,
		.i = 3,
	};

	a.c = 2;
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


plan_9()
{
	extern void f(U *a);
	struct D *d;
	f(d); // converts to U *

	typedef struct { int k; } mem_alias;
	struct
	{
		int hello;
		mem_alias;
	} st;

	return st.mem_alias.k;
}
