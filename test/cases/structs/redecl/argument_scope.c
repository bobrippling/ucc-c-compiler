// RUN: %check -e %s

struct A;

struct A a; // no error - tenative

struct A
{
	int i;
};


void f(void)
{
	struct B; // CHECK: note: forward declared here

	struct B b; // CHECK: error: "b" has incomplete type 'struct B'

	struct B
	{
		int i;
	};
}

int g(struct B *b) // CHECK: warning: declaration of 'struct B' only visible inside function
{
	struct B
	{
		int i;
	};

	return b->i; // not error - declaration is above
}

int h(struct C { int i; } *c) // CHECK: warning: declaration of 'struct C' only visible inside function
// CHECK: ^ note: previous definition here
{
	struct C {int i;}; // CHECK: error: redefinition of struct in scope
	return c->i;
}
