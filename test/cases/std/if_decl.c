// RUN: %ocheck 2 %s

main()
{
#include "../ocheck-init.c"
	if((struct A {int i;} *)0, 1){
		struct A a;
		a.i = 2;
		return a.i;
	}
	return 0;
}
