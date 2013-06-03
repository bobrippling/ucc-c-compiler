struct half_bytes
{
	int f_1;
	int bf_1 : 3;
};

main()
{
	struct half_bytes a;

	*(long *)&a = 0;

	a.f_1 = -1UL;
	a.bf_1 = -1UL;

	return a.bf_1;
}
