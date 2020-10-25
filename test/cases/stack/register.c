// RUN: %ucc -S -o- %s | grep 'subq \$[0-9]\+, %%rsp'
main()
{
	extern int i __asm("hi");
	register int j __asm("yo"); // ensure we get stack space for this
	i = 2;
	j = 3;
}
