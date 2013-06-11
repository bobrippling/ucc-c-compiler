struct bug_entry
{
	const char *desc;
};

main()
{
	do{
		asm volatile(
				"1:ud2\n"
				".pushsection __bug_table,\"a\"\n"
				"2:.long 1b - 2b, %0 - 2b\n"
				".word %1, 0\n"
				".org 2b+%2\n"
				".popsection"
				:
				: "i" ("<stdin>"),
				  "i" (29),
				  "i" (sizeof(struct bug_entry)));
			unreachable();
	}while (0);
}
