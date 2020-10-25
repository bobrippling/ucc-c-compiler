// RUN: %ocheck 0 %s

f(a, b, c)
{
	return a == b == c;
}

assert(b)
{
	if(!b){
		void abort(void);
		abort();
	}
}

main()
{
	assert(0 == f(0, 0, 0));
	assert(1 == f(0, 0, 1));
	assert(1 == f(0, 1, 0));
	assert(0 == f(0, 1, 1));
	assert(1 == f(1, 0, 0));
	assert(0 == f(1, 0, 1));
	assert(0 == f(1, 1, 0));
	assert(1 == f(1, 1, 1));

	return 0;
}
