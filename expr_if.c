#define NULL (void *)0
main()
{
	struct A { int i; };
	void *p = (__typeof(5)*)5;

	printf("%d %d\n", ((struct { int i; } *)p)->i, (p ? NULL : (struct{int i;}*)3)->i);

	(1 ? (struct A *)0 :          NULL)->i;
	(1 ? NULL          : (struct A *)0)->i;
}
