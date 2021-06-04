// RUN: %ocheck 9 %s

f(int i)
{
	return i - 2;
}

main()
{
#include "../ocheck-init.c"
	return ({
			int i = 5;
			3 + f(i + 1);
		}) + 2;
}
