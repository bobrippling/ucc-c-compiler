// RUN: %check %s
// RUN: %layout_check %s

typedef struct
{
	int i, j;
} A;

A x = { 1, 2 };
A y = { 1, 2, 3 };    // CHECK: /warning: excess initialiser/
A z = { 1, 2, 3, 4 }; // CHECK: /warning: excess initialiser/

A many[] = { 1, 2, 3, 4 }; // CHECK: !/warning: excess/
A more[] = { { 1 }, 2, 3, 4 }; // CHECK: !/warning: excess/
A two_a[2] = { { 1 }, 2, 3, }; // CHECK: !/warning: excess/
A two_b[2] = { { 1 }, 2, 3, 4 }; // CHECK: /warning: excess initialiser/
