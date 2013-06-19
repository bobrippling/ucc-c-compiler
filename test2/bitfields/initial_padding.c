// RUN: %ucc -o %t %s
// RUN: %t

// gcc and clang compatible - tcc isn't
main()
{
	struct
	{
			int : 3;
			int f : 2;
			// int : 27;
			int a;
	} a;

	memset(&a, 0, sizeof a);

	a.f = 3; // all bits
	a.a = 3;
	// 00011[pad-to-32], [30 0:s]11
	// ^anon
	//    ^f             ^a

	// XXX: this test assumes little-endian
	// read as:
	// [30-0:s]1100000000000000000000000000011000
	// ^--------^^------------------------------^
	//   high word   low word
	//                               8
	// 1100000000000000000000000000011000
	//                      ^4096
	// 16+8+some_large_number

	long long l = *(long long *)&a;
	printf("%lld\n", l);
	if(l != 12884901912)
		return 1;
	return 0;
}
