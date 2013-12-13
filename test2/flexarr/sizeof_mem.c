// RUN: %check -e %s
struct A
{
	int i;
	int vals[];
};

main()
{
	struct A x;
	return sizeof(x.vals); // CHECK: /error: sizeof incomplete type int\[\]/
}
