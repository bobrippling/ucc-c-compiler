// RUN: %ucc -o %t %s
// RUN: %t; [ $? -eq 6 ]

main()
{
	//int x[] = { 1, 2 };
	int x = {{{ 1 }}};
	int y = 2, k = 3;

	return x + y + k;
}
