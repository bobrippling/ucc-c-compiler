// RUN: %check %s
// RUN: %ocheck 1 %s

enum A; // CHECK: /warning: forward-decl/

f(enum A x);

enum A
{
	X, Y, Z
};

main()
{
#include "../ocheck-init.c"
	return f(Z);
}

f(enum A x)
{
	return x == 2;
}
