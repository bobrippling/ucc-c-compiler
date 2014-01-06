struct A
{
	int i;
	struct A *next; // BUG: recursive debug type creation
};

main()
{
	struct A a = {
		.next = &a,
		.i = 3,
	};

	for(struct A *p = a.next;
			p && p->i;
			p = p->next)
	{
		printf("%d\n", p->i--);
	}

	return 5;
}
