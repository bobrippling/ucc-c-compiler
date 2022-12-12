// RUN: %ocheck 2 %s
void f_void(){}
int f_int(){return 2;}
main()
{
#include "../ocheck-init.c"
	int i;

	i = f_int(), f_void();

	return i;
}
