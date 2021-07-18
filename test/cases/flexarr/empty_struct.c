// RUN: %ocheck 0 %s
// RUN: %check %s
struct A
{
	struct B {} b; // CHECK: /warning: struct is empty/
	int x[];       // CHECK: !/warning/
};

main()
{
#include "../ocheck-init.c"
	return sizeof(struct A);
}
