// RUN: %check -e %s

struct A // CHECK: note: from here
{
	int i;
};

g()
{
	enum A { X } a; // CHECK: !/error/
}

f(enum A { X } a) // CHECK: error: redefinition of struct as enum
{
	return a+1;
}
