// RUN: %check -e %s

struct A {
	int x; // CHECK: note: duplicate of this member
	int x; // CHECK: error: struct A contains duplicate member "x"
};
