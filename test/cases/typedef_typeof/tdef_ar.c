// RUN: %check -e --only %s -DERROR -std=c2x
// RUN: %ucc -fsyntax-only %s
//
// RUN: %layout_check %s

typedef int Flex[];
Flex;

// --------------------------


void error()
{
	typedef int A[];

	A x // CHECK: error: "x" has incomplete type 'A {aka 'int[]'}'
#ifndef ERROR
		= { 1 }
#endif
	;
}

// --------------------------

Flex ent1 = { 1, 2 },
		ent2 = { 3, 4, 5 };

// --------------------------

struct A {
	short n;
	// pad of 2
	Flex ar;
};

struct B {
	int n;
	// no pad
	Flex ar;
};

void ok()
{
	static struct A x = { 5, { 1, 2, 3 } }; // CHECK: warning: initialisation of flexible array (GNU)
	_Static_assert(sizeof x == sizeof(int));

	_Static_assert(sizeof(struct A) == sizeof(int));
}
