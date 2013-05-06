// RUN: %ucc -o %t %s
// RUN: %t; [ $? -eq 2 ]

main()
{
	int (*p)[];

	p = &(int[]){1, 2};

	return (*p)[1];
}
