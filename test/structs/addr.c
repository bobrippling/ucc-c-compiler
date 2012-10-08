#define NULL (void *)0

int *f(struct { int i, j; } *a)
{
	return &(*a).j;
}


main()
{
	if(f(NULL) != (void *)4)
		return 1;

	return 0;
}
