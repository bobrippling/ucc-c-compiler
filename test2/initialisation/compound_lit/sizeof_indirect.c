// RUN: %ocheck 0 %s

main()
{
	static int x = sizeof(int *){ (int[]){ 1,2,3 } };
	if(x != sizeof(int *))
		abort();
	return 0;
}
