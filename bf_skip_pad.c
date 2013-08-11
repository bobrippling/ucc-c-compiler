struct A
{
	int : 10;
	int : 0;
	int a;
	int : 0;
	int b;
};

struct A a = { 1, 2 };

pa(struct A *pa)
{
	for(int i = 0; i < 3; i++)
		printf("a[%d] = %d\n", i, i[(int *)pa]);
}

main()
{
	struct A local = { 1, 2 };

	pa(&local);
	printf("---\n");
	pa(&a);
}
