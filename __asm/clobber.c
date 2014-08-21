main()
{
	__asm("" : : :
			"memory", "cc",
			"rax", "ebx", "cx", "dl", "ah");
}
