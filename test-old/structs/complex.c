struct A
{
	int i;
	struct A *next;
};

main()
{
	struct A a, *pa = &a, **ppa = &pa;

	ppa = &(*ppa)->next;
}
