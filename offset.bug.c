main()
{
	extern int i asm("hi");
	register int j asm("yo");
	i = 2;
	j = 3;
}
