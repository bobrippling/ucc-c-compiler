// RUN: %ocheck 7 %s

long long l = 2;

main()
{
	l += 5LL;

	return l;
}
