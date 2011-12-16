int f()
{
	return 3;
}

main()
{
	int i;
	i = f() > 2;

	if(i != 1){
		dprintf(2, "error in assign order\n");
		return 1;
	}

	return 0;
}
