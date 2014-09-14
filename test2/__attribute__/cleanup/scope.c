// RUN: %ocheck 0 %s
// RUN: %check %s

int x;
cleanup(int *pint)
{
	x++;
}

assert(_Bool b)
{
	if(!b)
		abort();
}

typedef int __attribute__((cleanup(cleanup))) cleanup_int; // CHECK: !/warn/

f()
{
	cleanup_int i = 0;

	assert(x == 0);

	{
		cleanup_int j = 0;

		assert(x == 0);

		{
			cleanup_int k = 0;

			assert(x == 0);
		}
		assert(x == 1);
	}
	assert(x == 2);
}

main()
{
	f();
	assert(x == 3);

	return 0;
}
