print_ar(int a[], int n)
{
	while(n --> 0)
		printf("%d\n", *a++);
}

main()
{
	int x[] = { 4, 3, 2, 1 };
	char s[] = "hi there\n";

	print_ar(x, sizeof x / sizeof *x);

	printf("'%s'\n", s);
}
