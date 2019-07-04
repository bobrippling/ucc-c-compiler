// RUN: %ucc -fsyntax-only %s

main()
{
	__asm("" : : :
			"memory", "cc",
			"rax", "ebx", "cx", "dl", "ah",
			"r11");
}
