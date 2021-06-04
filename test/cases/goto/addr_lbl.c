// RUN: %ocheck 3 %s

main()
{
#include "../ocheck-init.c"
	void *p = &&x;

rejmp:
	goto *p;
x:
	p = &&y;
	goto rejmp;
y:
	return 3;
}

f()
{
y:;
}
