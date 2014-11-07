struct A
{
	unsigned a : 1;
	unsigned b : 6;
};

void *memset(void *, int, unsigned long);

main()
{
	struct A a;

	memset(&a, 1, sizeof a);

	a.a = 3;

	printf("%d\n", a.a);

	int i = 5;

	a.b = a.a = 7;

	printf("%d %d %d\n", i, a.a, a.b);
}
