// RUN: %ucc -o %t %s
// RUN: %t | grep -- -

main()
{
	if(1){
		int neg = '-';
		write(1, &neg, 1);
	}
	printf("\n");

	return 0;
}
