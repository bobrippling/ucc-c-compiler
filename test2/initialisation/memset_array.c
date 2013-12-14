// RUN: %ocheck 0 %s

struct A
{
	int x[2];
};

main()
{
	struct A a = {};

	if(a.x[0] || a.x[1])
		abort();

	return 0;
}
