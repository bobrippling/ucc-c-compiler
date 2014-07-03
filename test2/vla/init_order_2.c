// RUN: %ocheck 6 %s

main()
{
	int i = 5;
	int x[i][++i];

	return i;
}
