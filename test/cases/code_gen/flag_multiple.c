// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

f(int a, int b, int c)
{
	return a == b & b == c;
}

assert(x)
{
	if(!x)
		abort();
}

main()
{
	assert(f(1, 2, 3) == 0);
	assert(f(1, 1, 3) == 0);
	assert(f(1, 1, 1) == 1);
	assert(f(1, 1, 1) == 1);
}
