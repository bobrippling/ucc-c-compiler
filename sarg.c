typedef struct A
{
	long i, j, k;
} A;

f(A a)
#ifdef IMPL
{
	return a.i + a.j + a.k;
}
#else
;
#endif

#ifndef IMPL
main()
{
	A a = { 1, 2, 3 };

	return f(a);
}
#endif
