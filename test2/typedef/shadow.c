// RUN: %check -e %s

typedef unsigned long size_t;

main()
{
	typedef void *size_t; // CHECK: /warning: shadowing definition of size_t, from:/

	size_t p = 5;

	return p;
}
