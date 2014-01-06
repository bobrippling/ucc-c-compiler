extern void b(int *);

main()
{
	int i;
	b(&i);
	return i;
}
