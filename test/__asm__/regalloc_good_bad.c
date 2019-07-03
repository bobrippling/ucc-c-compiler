// RUN: %check -e %s

// with clobber:  1 1 1 1 0 0 0 0
// "d" allocated: 1 1 0 0 1 1 0 0
// "r" or "q":    r q r q r q r q

test_0()
{
	__asm("" ::
				"a"(0),
				"b"(0),
				"c"(0),
				"S"(0),
				"D"(0),
				"d"(0), // CHECK: error: input constraint unsatisfiable
				"r"(5)
			: "rdx"); // clobbered here
}
test_1()
{
	__asm("" ::
				"a"(0),
				"b"(0),
				"c"(0),
				"S"(0),
				"D"(0),
				"d"(0), // CHECK: error: input constraint unsatisfiable
				"q"(5)
				: "rdx"); // clobbered here
}
test_2()
{
	__asm("" :: // CHECK: !/error/
				"a"(0), // CHECK: !/error/
				"b"(0), // CHECK: !/error/
				"c"(0), // CHECK: !/error/
				"S"(0), // CHECK: !/error/
				"D"(0), // CHECK: !/error/
				"r"(5) // CHECK: !/error/
				// ^ r8 goes into here, for e.g. - this will fail on 32-bit
			: "rdx"); // CHECK: !/error/
}
test_3()
{
	__asm("" ::
				"a"(0),
				"b"(0),
				"c"(0),
				"S"(0),
				"D"(0),
				"q"(5) // CHECK: error: input constraint unsatisfiable
				: "rdx"); // clobbered here
}


test_4()
{
	__asm("" :: // CHECK: !/error/
				"a"(0), // CHECK: !/error/
				"b"(0), // CHECK: !/error/
				"c"(0), // CHECK: !/error/
				"S"(0), // CHECK: !/error/
				"D"(0), // CHECK: !/error/
				"d"(0), // CHECK: !/error/
				"r"(5)); // CHECK: !/error/
	// again - r8
}
test_5()
{
	__asm("" ::
				"a"(0),
				"b"(0),
				"c"(0),
				"S"(0),
				"D"(0),
				"d"(0),
				"q"(5)); // CHECK: error: input constraint unsatisfiable
}
test_6()
{
	__asm("" :: // CHECK: !/error/
				"a"(0), // CHECK: !/error/
				"b"(0), // CHECK: !/error/
				"c"(0), // CHECK: !/error/
				"S"(0), // CHECK: !/error/
				"D"(0), // CHECK: !/error/
				"r"(5)); // CHECK: !/error/
	// "r" => rdx
}
test_7()
{
	__asm("" :: // CHECK: !/error/
				"a"(0), // CHECK: !/error/
				"b"(0), // CHECK: !/error/
				"c"(0), // CHECK: !/error/
				"S"(0), // CHECK: !/error/
				"D"(0), // CHECK: !/error/
				"q"(5)); // CHECK: !/error/
	// "r" => rdx
}
