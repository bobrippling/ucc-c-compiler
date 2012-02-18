struct A
{
	int a, b;
};

main()
{
#define O(x) (long)(&((struct A *)0)->x)
	printf("a = %lx, b = %lx, diff = %ld\n", O(a), O(b), O(b) - O(a));
}
