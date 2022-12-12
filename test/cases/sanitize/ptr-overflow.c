// RUN: %ocheck 2 %s -fsanitize=pointer-overflow -fsanitize-error=call=san_fail

#define LONG_MAX __LONG_MAX__

void san_fail(void)
{
	_Noreturn void exit(int);
	exit(2);
}

main()
{
#include "../ocheck-init.c"
	int *max = (int *)(LONG_MAX - sizeof(int) + 1);

	max++;

	return 0;
}
