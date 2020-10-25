// RUN: %ocheck 0 %s
// RUN: %check %s
void abort(void) __attribute__((noreturn));

main()
{
	static struct
	{
		int n;
		struct
		{
			char *a;
			int z;
		} x[];
	} abc = {
		2,
		.x[2].a = "hi" // CHECK: /warning: initialisation of flexible array/
	};

	if(sizeof(abc) != 8)
		abort();

	if(abc.n != 2)
		abort();

	return 0;
}
