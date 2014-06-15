// RUN: %ucc %s

typedef long ssize_t;
__typeof(__typeof(ssize_t)) g;

main()
{
	typedef long unsigned size_t;
	__typeof(__typeof(size_t)) a;
}
