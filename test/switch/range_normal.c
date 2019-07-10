// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

map(int i)
{
	switch(i){
		case 1 ... 3:
			return 1;

		case 4:
			return 2;

		default:
			return 3;
	}
}

assert(_Bool b)
{
	if(!b)
		abort();
}

main()
{
	assert(map(1) == 1);
	assert(map(2) == 1);
	assert(map(3) == 1);
	assert(map(4) == 2);

	assert(map(5) == 3);
	assert(map(50) == 3);
	assert(map(-2) == 3);

	return 0;
}
