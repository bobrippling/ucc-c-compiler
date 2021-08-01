// RUN: %ucc -c %s
typedef unsigned long size_t;

f()
{
	typedef short size_t;
	size_t x;

	_Static_assert(sizeof(x) == 2, "short typedef missed");
}
