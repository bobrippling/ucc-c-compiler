// RUN: %ucc -o %t %s && %t
enum
{
	a,
	b,
	c,
	d = b,
	e,
	f
};

#define assert(x) if(!(x)) return 1

main()
{
	assert(a == 0);
	assert(b == 1);
	assert(c == 2);
	assert(d == 1);
	assert(e == 2);
	assert(f == 3);

	return 0;
}
