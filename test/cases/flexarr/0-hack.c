// RUN: %check %s
struct A
{
	int i;
	short ents[0]; // CHECK: /note: array declared here/
	// zlas are a GNUC extension
};

main()
{
	struct A x;
	x.i = 2;
	x.ents[0] = 5; // CHECK: /warning: index 0 out of bounds of array, size 0/
}
