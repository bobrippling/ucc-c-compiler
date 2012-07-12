main()
{
	int i;
	i = 0;
start:
	if(i == 5)
		goto fin;
	printf("i = %d\n", i);
	i++;
	goto start;
fin:
	return 3;
}
