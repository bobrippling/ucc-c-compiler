// RUN: %check %s -fshow-inlined -finline-functions -fno-semantic-interposition
// RUN: %ocheck 3 %s -finline-functions -fno-semantic-interposition

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
