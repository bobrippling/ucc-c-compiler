main()
{
	int i;

	for(i = 0; i < 10; i++){
		printf("for, i=%d\n", i);
		if(i == 5)
			break;
	}

	while(i < 10){
		i++;
		printf("while, i=%d\n", i);
		if(i == 6 || i == 7)
			continue;
		break;
	}

	do{
		printf("hello\n");
		continue;
	}while(0);


	for(i = 0; i < 10; i++){
		int j;
		for(j = 0; j < 10; j++){
			printf("%d %d\n", i, j);
			if(j == 5)
				break;
		}
	}

	for(i = 0; i < 30; i++){
		switch(i){
			case 1:
				break;
			case 2:
				continue;
			default:
				break;
		}
		printf("not 2 = %d\n", i);
	}
}
