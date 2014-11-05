// RUN: %check %s -fshow-inlined -finline-functions
// RUN: %ocheck 3 %s -finline-functions

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
