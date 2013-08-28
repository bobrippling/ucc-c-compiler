// RUN: %check -e %s

struct A;

int f()
{
	struct A a; // CHECK: /error: .*incomplete/
	//a.i = 2;
	//return a.i;
}

struct A // this doesn't complete struct A for f()'s scope
{
	int i;
};
