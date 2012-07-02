main(int argc, char **argv)
{
	printf("argc = %d. ", argc);
	switch(argc){
		case 1 ... 3:
			puts("1 to 3 args");
			break;
		case 4:
			puts("4 args");
			break;
		default:
			printf("%d args - default\n");
			break;
	}
}
