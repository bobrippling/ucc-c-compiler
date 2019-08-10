// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));
enum
{
	a,
	b,
	c,
	d = b,
	e,
	f
};

#define assert(x) if(!(x)) abort()

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
