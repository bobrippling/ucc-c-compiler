struct A
{
	int i;
	int vals[];
};

main()
{
	struct A x;
	return sizeof(x.vals);
}
