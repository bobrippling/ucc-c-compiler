// RUN: %check %s

struct A
{
	int i;
};

g()
{
	enum A { X } a;
}

f(enum A { X } a) // CHECK: warning: declaration of 'enum A' only visible inside function
{
	return a+1;
}
