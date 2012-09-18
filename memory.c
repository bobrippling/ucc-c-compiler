int glob;

main()
{
	int local;
	int *p;

	local = 2;

	glob = local;

	*(short *)5 = 3;

	*p = 7;
}
