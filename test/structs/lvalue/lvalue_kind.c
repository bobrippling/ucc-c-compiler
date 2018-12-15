// RUN: %ocheck 3 %s
// RUN: %ocheck 3 %s -std=c89

struct A
{
	int ar[2];
	int other;
};

struct A a(void)
{
	return (struct A){ { 1 }, 3 };
}

main()
{
	int o = a().other;

	if(o != 3)
		abort();

	return a().other;
}
