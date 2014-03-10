static int g();
static void f() __attribute__((cleanup(g)));

static int x __attribute__((cleanup(g)));

clean(int *p __attribute__((cleanup(f))))
{
	printf("clean! @ %p\n", p);
	*p = 1;
}

main()
{
	int y() __attribute__((cleanup(clean)));
	static int st __attribute__((cleanup(clean)));

	int x __attribute__((cleanup(clean))) = 3;

	for(int i __attribute((cleanup(clean))) = 0; i < 3; i++)
		;

	return x;
}
