struct A;

struct A
{
	int i;
};

main()
{
	struct A *p;

	p = (struct A *)2;

	p->i = 5;
}
