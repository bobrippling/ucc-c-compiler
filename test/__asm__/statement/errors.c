// RUN: %check --only -e %s

void f(int i)
{
	{
		__asm("" // CHECK: error: unknown entry in clobber: "hi"
				: "=r"(i)
				:
				: "hi");
		// previously we would release i's out_val twice, causing a crash
	}

	{
		int i = 3;

		__asm("%0 <-- %1"
				: "=r"(i)
				: "b+"(5)); // CHECK: error: modifier character not at start ('+')
	}

	{
		int i = 3;

		// bad outputs
		__asm("" : ""(i)); // CHECK: error: output operand missing =/+ in constraint
		__asm("" : "&"(i)); // CHECK: error: output operand missing =/+ in constraint

		// bad inputs
		__asm("" : : ""(0));   // CHECK: error: input constraint unsatisfiable
		__asm("" : : "="(0));  // CHECK: error: input operand has =/+ in constraint
		__asm("" : : "&"(0));  // CHECK: error: constraint modifier '&' on input
		__asm("" : : "&r"(0)); // CHECK: error: constraint modifier '&' on input
	}

	{
		__asm("" : : : "rbz"); // CHECK: error: unknown entry in clobber: "rbz"
	}

	{
#define LIMIT(c, n) __asm("" : : c(n))
		LIMIT("I", 5); // no error
		LIMIT("I", 31); // no error
		LIMIT("I", -1); // CHECK: error: input constraint unsatisfiable
		LIMIT("I", 32); // CHECK: error: input constraint unsatisfiable

		LIMIT("J", 5); // no error
		LIMIT("J", 63); // no error
		LIMIT("J", -1); // CHECK: error: input constraint unsatisfiable
		LIMIT("J", 64); // CHECK: error: input constraint unsatisfiable

		LIMIT("K", 5); // no error
		LIMIT("K", -3); // no error
		LIMIT("K", 129); // CHECK: error: input constraint unsatisfiable
		LIMIT("K", -129); // CHECK: error: input constraint unsatisfiable

		LIMIT("M", -1); // CHECK: error: input constraint unsatisfiable
		LIMIT("M", 0); // no error
		LIMIT("M", 1); // no error
		LIMIT("M", 2); // no error
		LIMIT("M", 3); // no error
		LIMIT("M", 4); // CHECK: error: input constraint unsatisfiable

		LIMIT("N", 0); // no error
		LIMIT("N", 256); // no error
		LIMIT("N", -1); // CHECK: error: input constraint unsatisfiable
		LIMIT("N", 257); // CHECK: error: input constraint unsatisfiable
#undef LIMIT
	}

	{
		__asm("mov $5, %0" : "=i"(*(int *)3)); // CHECK: error: 'i' constraint in output
	}

	{
		__asm__ volatile ("");
		__asm__ const (""); // CHECK: Non-volatile qualification after asm (const)
		__asm__ __const (""); // CHECK: Non-volatile qualification after asm (const)

		__asm__ volatile __volatile__ (""); // CHECK: Duplicate qualification after asm (volatile)
	}

	{
		// run out of non-callee-save regs:
		__asm("addl %1, %0"
				: "+r"(i)
				: "r"(3), "r"(3), "r"(3), "r"(3), "r"(3), "r"(3), "r"(3)
				, "r"(3), "r"(3), "r"(3), "r"(3), "r"(3), "r"(3)
				, "r"(3) // CHECK: error: input constraint unsatisfiable
				);
	}

	{
		__asm("%0 %1"
				:
				: "c"(3), "c"(5)); // CHECK: error: input constraint unsatisfiable
		// 'c' has to be 3 and 5
	}

	{
		struct A { long a, b, c; } a;
		__asm("" : "=r"(a) : "r"(a)); // CHECK: error: struct involved in __asm__() output
	}
}


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
	__asm("" :: // no error
				"a"(0), // no error
				"b"(0), // no error
				"c"(0), // no error
				"S"(0), // no error
				"D"(0), // no error
				"r"(5) // no error
				// ^ r8 goes into here, for e.g. - this will fail on 32-bit
			: "rdx"); // no error
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
	__asm("" :: // no error
				"a"(0), // no error
				"b"(0), // no error
				"c"(0), // no error
				"S"(0), // no error
				"D"(0), // no error
				"d"(0), // no error
				"r"(5)); // no error
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
	__asm("" :: // no error
				"a"(0), // no error
				"b"(0), // no error
				"c"(0), // no error
				"S"(0), // no error
				"D"(0), // no error
				"r"(5)); // no error
	// "r" => rdx
}
test_7()
{
	__asm("" :: // no error
				"a"(0), // no error
				"b"(0), // no error
				"c"(0), // no error
				"S"(0), // no error
				"D"(0), // no error
				"q"(5)); // no error
	// "r" => rdx
}
