// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

f(long l)
{
	switch(l){
		case 3:
			return 1;
		case 2L:
			return 2;
	}
	return 3;
}

assert(_Bool b)
{
	if(!b)
		abort();
}

main()
{
	assert(f(-1) == 3);
	assert(f(0) == 3);
	assert(f(1) == 3);
	assert(f(2) == 2);
	assert(f(3) == 1);
	assert(f(4) == 3);
	assert(f(5) == 3);

	return 0;
}
