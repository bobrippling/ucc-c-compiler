// RUN: %ocheck 0 %s
// RUN: %check %s
struct A
{
	struct B {} b; // CHECK: /warning: struct is empty/
	int x[];       // CHECK: !/warning/
};

main()
{
	return sizeof(struct A);
}
