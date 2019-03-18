// RUN: %ocheck 0 %s
// RUN: %check %s

struct A
{
	struct B
	{
		int i, j;
	} b;
	int k;
};

main()
{
	struct B b = { 1, 2 };
	struct A a = {
		.b = b,  // CHECK: note: previous initialisation
		.b.i = 2 // CHECK: warning: designating into object discards entire previous initialisation
	};

	if(b.i != 1 || b.j != 2)
		abort();

	if(a.b.i != 2)
		abort();
	if(a.b.j != 0)
		abort();
	if(a.k != 0)
		abort();

	return 0;
}
