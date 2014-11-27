// RUN: %check -e %s

f(int i)
{
	__asm("" // CHECK: error: unknown entry in clobber: "hi"
			: "=r"(i)
			:
			: "hi");

	// previously we would release i's out_val twice, causing a crash
}
