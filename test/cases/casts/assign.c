// RUN: %check %s

main()
{
	int i;
	short s;

	s = i = 3; // CHECK: !/warn/

	// s = (short)(i = 3)

	return s + i;
}
