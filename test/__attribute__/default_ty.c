// RUN: %ocheck 3 %s

main()
{
	__attribute(()) x = 3;
	return x;
}
