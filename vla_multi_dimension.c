extern int printf(const char *, ...) __attribute((format(printf, 1, 2)));

g()
{
	printf("g()!\n");
	return 3;
}

h()
{
	printf("h()!\n");
	return 2;
}

void show(int (*p)[*])
{
	printf("%p\n", p);
}

void f(int n)
{
	int ar[n * g()][h()]; // 4 * 2 * 3 * n = 24 * n
	// 24 * 3 = 72

	show(ar);
}

main()
{
	f(3); // 3 * 4 = 12
}
