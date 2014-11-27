// RUN: %ucc -fsyntax-only %s

main()
{
	__asm(""
			:
			);

	__asm(""
			:
			:
			);

	__asm(""
			:
			:
			:
			);
}
