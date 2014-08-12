// RUN: %check %s -fshow-inlined
// RUN: %ocheck 3 %s

main()
{
	return - ^int (int x)
	{ // CHECK: note: function inlined
		return ^__attribute((noinline)) (int x) // CHECK: !/inline/
		{ // CHECK: !/inline/
			return -x;
		}(x + 1);
	}(2);
}
