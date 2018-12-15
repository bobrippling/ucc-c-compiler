main()
{
	int i = {{{ 5 }}}, j = 2, k = { 1, 2 };
	//int a[] = { 1, 2, 3 };

	if(i != 5)
		abort();
	if(j != 2)
		abort();
	if(k != 1)
		abort();

	return 0;
}
