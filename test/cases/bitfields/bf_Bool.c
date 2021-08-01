// RUN: %ocheck 1 %s

main()
{
#include "../ocheck-init.c"
	struct
	{
		_Bool x : 5;
	} a;
	volatile int three = 3; // prevent constant conversion

	a.x = three;

	return a.x; // 1
}
