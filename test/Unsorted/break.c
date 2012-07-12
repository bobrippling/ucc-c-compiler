main()
{
	int i = 0;
	while(1){
		printf("while, i = %d\n", i);
		if(i == 5)
			break;
		i++;
	}

	for(;; i++)
		if(i < 10)
			printf("for, i = %d\n", i);
		else
			break;

	do{
		printf("do, i = %d\n", i);
		if(i++ == 15)
			break;
	}while(1);

	return 0;
}
