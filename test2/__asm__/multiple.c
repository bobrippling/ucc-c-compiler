main()
{
	asm("call __Z4sizeIiEiv\n"
			"mov %%eax, %0\n"
			"jmp .after_tim\n"
			".LSTR_tim: .byte 104, 105, 32, 37, 100, 0\n"
			".after_tim:\n"
			"pushl .LSTR_tim\n"
			"pushl %%eax\n"
			"call _printf\n"
			"add $8, %%esp\n"
			: "=r"(i));
}
