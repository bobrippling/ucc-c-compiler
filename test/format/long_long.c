// RUN: %check %s

main()
{
	int printf(const char *, ...)
		__attribute((format(printf, 1, 2)));

	long long ll = 0;

  printf("%lld\n", ll); // CHECK: !/warn/
}
