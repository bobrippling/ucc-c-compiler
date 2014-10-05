// RUN: %ocheck 3 %s

main()
{
	int i;
	({ i; }) = 3;
	return i;
}
