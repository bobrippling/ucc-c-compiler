int f()
{
	return 3;
}

main()
{
	int i;
	i = f() > 2;

	if(i != 1){
		abort();
	}

	return 0;
}
