// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));
assert(int cond)
{
	if(!cond)
		abort();
}

main()
{
	_Bool x = 1;
	assert(x == 1);

	x++;
	assert(x == 1);

	++x;
	assert(x == 1);

	x += 1;
	assert(x == 1);

	return 0;
}
