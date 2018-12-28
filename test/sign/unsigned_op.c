// RUN: %ocheck 0 %s

main()
{
	return -5 < 10U; // unsigned cmp, false
}
