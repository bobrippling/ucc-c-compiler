strcmpany(char *a, char *b)
{
	return (a[0] == b[0] && a[1] == b[1])
	    || (a[0] == b[1] && a[1] == b[0]);
}

main()
{
	printf("hex dec\n");
	for(int i = 1; i < 100; i++){
		char buf[2][4];

		snprintf(buf[0], sizeof buf[0], "%x", i);

		if(strpbrk(buf[0], "abcdef"))
			continue;

		snprintf(buf[1], sizeof buf[1], "%d", i);

		printf("%c %s %s\n",
				"-#"[strcmpany(buf[0], buf[1])],
				buf[0], buf[1]);
	}
}
