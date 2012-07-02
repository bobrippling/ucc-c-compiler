main(int argc, char **argv)
{
	switch(argc){
		case 2:
			printf("argc is 2\n");
			switch(strlen(argv[1])){
				case 0:
					printf("strlen(argv[1]) is 0\n");
				case 1:
					printf("strlen(argv[1]) is 1\n");
				default:
					printf("strlen(argv[1]) = %d\n", strlen(argv[1]));
			}
		case 1:
			printf("argc 2 or 1\n");
	}
	return 0;
}
