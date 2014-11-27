// RUN: %check -e %s

main()
{
	__asm("" : : : "rbz"); // CHECK: error: unknown entry in clobber: "rbz"
}
