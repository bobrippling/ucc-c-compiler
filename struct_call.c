struct A
{
	int i, j, k;
	float f;
};

f(struct A a)
#ifdef I
{
	return a.i + a.j + a.k;
}
#else
;
#endif

main()
{
	struct A a = { 1, 2, 3 };
	f(a);
}
