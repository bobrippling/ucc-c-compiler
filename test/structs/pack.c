struct A
{
	int a, b;
};

main()
{
#define O(x) (&((struct A *)0)->x)
	printf("a = %p, b = %p, diff = %d\n", O(a), O(b), O(b) - O(a));
}
