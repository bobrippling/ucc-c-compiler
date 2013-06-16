// RUN: %check -e %s
typedef unsigned long size_t;

g()
{
	size_t a; // CHECK: /error: mismatching definitions of "a"/
	typedef int a;
}
