static int *f(int *);

int *f(int *p)
{
	int *r = p;
	return r;
}

main()
{
	f((void *)0);
}
