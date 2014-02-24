struct A
{
	int i, j, k;
	float f;
};

f(struct A a)
#ifdef I
{
	return a.i + a.j + a.k + a.f;
}
#else
;
#endif

#ifndef I
main()
{
	struct A a = { 1, 2, 3, 4 };
	return f(a);
}
#endif
