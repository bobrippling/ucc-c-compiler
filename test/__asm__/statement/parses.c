// RUN: %ucc -fsyntax-only %s

int main()
{
	// clobbers:
	__asm("" : : :
			"memory", "cc",
			"rax", "ebx", "cx", "dl", "ah",
			"r11");

	// empties:
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
