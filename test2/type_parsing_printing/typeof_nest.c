// RUN: %ucc %s

typedef long ssize_t;
typeof(typeof(ssize_t)) g;

main()
{
	typedef long unsigned size_t;
	typeof(typeof(size_t)) a;
}
