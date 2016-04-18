// RUN: %ocheck 0 %s

#define assert(cond) if(!(cond)) return 1

int o_cnt, a_cnt;

o(float a, float b)
{
	o_cnt++;
	return a || b;
}

a(float a, float b)
{
	a_cnt++;
	return a && b;
}

main()
{
	assert( o(1, 2));
	assert( o(0, 2));
	assert( o(1, 0));
	assert(!o(0, 0));

	assert( a(1, 2));
	assert(!a(0, 2));
	assert(!a(1, 0));
	assert(!a(0, 0));

	assert(o_cnt == 4 && a_cnt == 4);

	return 0;
}
