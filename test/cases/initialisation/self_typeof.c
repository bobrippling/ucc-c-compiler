// RUN: %ocheck 3 %s

main()
{
	int i = (__typeof(i))3;

	return i;
}
