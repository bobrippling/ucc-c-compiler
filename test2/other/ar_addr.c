// RUN: %ucc %s -o %t
// RUN: %t; [ $? -eq 2 ]

main()
{
	int ar[10];
	int *p;

	p = &ar[5];

	*p = 2;

	return ar[5];
}
