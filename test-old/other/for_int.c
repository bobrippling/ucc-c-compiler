f()
{
	int lvl1 = 0;
	for(int lvl2 = 0; lvl2 < 5; lvl2++){
		int lvl3 = 2;
		printf("lvl2=%d\n", lvl2);
	}
	return lvl1;
}

main()
{
	f();

	for(int i = 0, j = 2; i < 5; i++){
		printf("i = %d, j = %d\n", i, j);
		j += i;
	}

	for(int i = 42, j = 84; ;){
		printf("i = %d 42, j = %d 84\n", i, j);
		break;
	}

	for(int i = 1, j, k = 2;;){
		printf("1 = %d, ? = %d, 2 = %d\n", i, j, k);
		break;
	}

	for(int i = 1, j = 2, k = 3;;)
		return i + j + k;
}
