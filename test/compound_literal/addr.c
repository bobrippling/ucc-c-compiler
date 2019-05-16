// RUN: %ocheck 2 %s

main()
{
	int *p = &(int){2};
	return *p;
}
