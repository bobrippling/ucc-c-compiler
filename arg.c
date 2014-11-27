struct A
{
	int i;
	float f;
	float *p;
};

f(struct A a)
{
	printf("%d %f %p\n", a.i, a.f, a.p);
}

/*
main()
{
	struct A a = { 1, 2, &a.f };

	printf("%p\n", &a.f);

	f(a);
}
*/
