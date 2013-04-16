main()
{
	extern int i asm("hi");
	register int j asm("yo"); // ensure we get stack space for this
	i = 2;
	j = 3;
}
