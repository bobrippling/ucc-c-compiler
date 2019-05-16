// RUN: %ocheck 8 %s

main()
{
	int a[2];

	return sizeof a; // 8
}
