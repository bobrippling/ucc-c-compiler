void *malloc(unsigned long);

void f(int *p)
{
	*p = 5;
}

int g(int *p)
{
	return *p;
}

void check_ptr(int *p)
{
	g(p);
}

main()
{
	void __asan_init_v4(void);
	__asan_init_v4();

	int *p = malloc(10);

	printf("p = %p\n", p);

	check_ptr(p); // good
	check_ptr(p - 1); // bad
	check_ptr(p + 1); // good
	check_ptr(p + 2); // bad
}
