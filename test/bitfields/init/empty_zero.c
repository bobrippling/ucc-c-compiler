// RUN: %ucc -c %s
// RUN: %layout_check %s
// RUN: %check %s
struct A
{
	int : 0;
};

struct A a;
struct A b = {};
struct A c = { 1 }; // CHECK: /warning: excess initialiser for 'struct A'/
struct A *d = &c;
struct A e[] = { {}, {}, {} };
