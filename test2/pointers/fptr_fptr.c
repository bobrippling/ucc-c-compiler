// RUN: %check %s

main()
{
	// must be different types for the cast to be generated
	void (*a)(int, int), (*b)(int) = 0;

	a = b; // CHECK: !/warning:.*function.*pointer/
}
