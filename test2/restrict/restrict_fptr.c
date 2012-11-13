// RUN: %ucc %s -S; [ $? -ne 0 ]

main()
{
	int (*restrict f)(void);
}
