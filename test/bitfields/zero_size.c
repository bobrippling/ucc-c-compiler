// RUN: %check %s
// RUN: %ucc -c -o %t %s

struct A
{
	int : 0;
};

struct A a; // CHECK: !/warn/
struct A b = {}; // CHECK: !/warn/
struct A c = { 1 }; // CHECK: warning: excess initialiser for 'struct A'
struct A *d = &c; // CHECK: !/warn/
struct A e[] = { {}, {}, {} }; // CHECK: !/warn/
