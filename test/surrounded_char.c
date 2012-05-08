typedef struct
{
	int i;
	char c;
	int j;
} A;

main()
{
	A a;

	a.i = 123456;
	a.c = 2;
	a.j = 123456;

	printf("%d %d %d\n", a.i, a.c, a.j);
}
