// RUN: %ocheck 0 %s

struct A
{
	int *p;
	int how;
};

main()
{
	static struct A a = {
		// ensure &a.how uses the correctly mangled name of 'a' (static)
		.p = &a.how
	};

	if(a.p != &a.how)
		abort();
	if(a.how)
		abort();
}
