main()
{
	int ent1[] = {
		[0 ... 5] = 5,
		[3] = 1,
		[4] = 2,
	};

	if(ent1[0] != 5
	|| ent1[1] != 5
	|| ent1[2] != 5
	|| ent1[3] != 1
	|| ent1[4] != 2
	|| ent1[5] != 5)
		abort();

	return 0;
}
