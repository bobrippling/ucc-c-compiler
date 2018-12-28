// RUN: %check %s

f(int) __attribute__((nonnull));

main()
{
	// shouldn't get null-passed warning
	f(0); // CHECK: !/warning/
	return 0;
}
