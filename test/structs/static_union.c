f()
{
	static union
	{
		int i, j;
	} x;
	static int q;
	int ret;

	if(q)
		ret = x.i++;
	else
		ret = x.j++;

	q = !q;

	return ret;
}

main()
{
	for(int i = 0; i < 5; i++)
		printf("%d\n", f());

	return 0;
}
