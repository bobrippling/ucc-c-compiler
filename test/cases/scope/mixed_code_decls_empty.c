// RUN: %ocheck 5 %s

static int r;

f()
{
	r = 5;
}

no_stmts()
{
	int i = f();
}

main()
{
#include "../ocheck-init.c"
	no_stmts();
	return r;
}
