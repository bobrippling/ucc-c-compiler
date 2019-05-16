// RUN: %ocheck 6 %s

main()
{
	//int x[] = { 1, 2 };
	int x = {{{ 1 }}};
	int y = 2, k = 3;

	return x + y + k;
}
