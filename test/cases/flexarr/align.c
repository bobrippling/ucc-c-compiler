// RUN: %ocheck 16 %s
struct A
{
	char x;
	double ar[];
};

main()
{
#include "../ocheck-init.c"
	return _Alignof(struct A) + sizeof(struct A);
}
