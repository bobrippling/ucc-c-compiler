// RUN: %check --only -e %s -fplan9-extensions

struct A
{
	int a_sub; // CHECK: note: duplicate of this member, and ...
};

typedef struct A A;

struct ambig // CHECK: error: struct ambig contains duplicate member "a_sub"
{
	A; // CHECK: warning: tagged struct 'A {aka 'struct A'}' is a Microsoft/Plan 9 extension
	int a_sub; // CHECK: note: ... this member
};
