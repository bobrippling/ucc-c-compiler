// RUN: %check -e --only %s

struct A
{
	int n;
	int vals[];
} x[] = { // CHECK: warning: embedded flexible-array as array element
	1, { 7 },    // CHECK: error: initialisation of nested flexible array
	// CHECK: ^/warning: missing braces/
	2, { 5, 6 }, // CHECK: error: initialisation of nested flexible array
	// CHECK: ^/warning: missing braces/
};

struct B {
	int x;
	char buf[];
};

struct B a = { .buf = "hello", .x = 3 }; // CHECK: warning: initialisation of flexible array (GNU)

struct B as[] = { // CHECK: warning: embedded flexible-array as array element
	{ .x = 1, .buf = "hi" }, // CHECK: error: initialisation of nested flexible array
	//        ^~~~~~~~~~~ member must be empty when in array
	{ .x = 2, .buf = "haiahhida" }, // CHECK: error: initialisation of nested flexible array
	{ .x = 3, .buf = "yoyo" }, // CHECK: error: initialisation of nested flexible array
};

char *f(int i) {
	return as[i].buf;
}

struct C {
	int x;
	struct B a; // CHECK: warning: embedded flexible-array
	// CHECK: ^warning: embedded struct with flex-array not final member
	int y;
} b = {
	.x = 1, .a = { .x = 2, }, .y = 3,
};

int main(void) {
	__auto_type k = sizeof("z");

	union {
		char b[k]; // CHECK: error: member has variably modifed type 'char[vla]'
	} w = { .b = "z" };
	//           ^~~ this would previously ICE because we were checking a flex-array init from char[]/strlit
}
