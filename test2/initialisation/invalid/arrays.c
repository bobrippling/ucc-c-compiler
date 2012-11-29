// RUN: %ucc %s; [ $? -ne 0 ]
main()
{
	static int x[] = 1;
	static int x[] = {1, 2};
	int y[] = 1;
}
