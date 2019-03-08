// RUN: %check --only %s

main()
{
	int printf(const char *, ...)
		__attribute((format(printf, 1, 2)));

	long long ll = 0;

  printf("%lld\n", ll);

  printf("%hhd\n", (signed char)3);
}
