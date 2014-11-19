// RUN: %ocheck 0 %s -std=c89

int called;

struct A
{
	int x[1];
} f()
{
	if(called)
		abort();
	called = 1;

	// ensure we can initialise arrays in C89 mode
	// i.e. no nonsense about rvalue-arrays
	return (struct A){
		{ 1 }
	};
}

main()
{
	struct A a = f();

	if(a.x[0] != 1)
		abort();

	return 0;
}
