main()
{
	// (compile with -m32)

	__asm(">%[hi]<"
			:
			:
				"a"(0),
				"b"(0),
				"c"(0),
				//"d"(0),
				"S"(0),
				"D"(0),

				[hi]"r"(5) // no register available for "r"(5) - 'd' would be but it's clobbered

			: "rdx");
}
