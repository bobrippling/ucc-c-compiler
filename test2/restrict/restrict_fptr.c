// RUN: %ucc %s -S -o %t; [ $? -ne 0 ]

main()
{
	int (*restrict f)(void);
}
