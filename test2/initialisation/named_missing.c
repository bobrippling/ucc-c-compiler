// RUN: %check %s

struct A
{
	int i, j, k;
} x = {
	1 // CHECK: warning: 2 missing initialisers for 'struct A'
	  // CHECK: ^ note: starting at "j"
};
