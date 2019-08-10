// RUN: %ocheck SIGILL %s -ftrapv -fno-const-fold -DT=int
// RUN: %ocheck SIGILL %s -ftrapv -fno-const-fold -DT=long
// RUN: %ocheck 0 %s -fno-const-fold -DT=int
// RUN: %ocheck 0 %s -fno-const-fold -DT=long

main()
{
	// test with T being both int and long, to check how truncations are dealt with
	T x;

	// ensure powers of two aren't shift-converted, as overflow can't catch this
	x = -3 * 0x4000000000000000;

	return 0;
}
