main()
{
	asm goto("1:"
			"jmp %l[lbl_nam]"
			: : "i"(var) : : lbl_nam);

lbl_nam:
	return 0;
}
