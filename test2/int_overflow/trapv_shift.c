// RUN: %ucc -ftrapv -fno-const-fold -o %t %s
// RUN: %t; [ $? -ne 0 ]

main()
{
	int x;

	// ensure powers of two aren't shift-converted, as overflow can't catch this
	x = -3 * 0x4000000000000000;

	return 0;
}
