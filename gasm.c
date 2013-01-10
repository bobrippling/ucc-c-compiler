asm(".globl a");
asm("a: .long 2");

main()
{
	extern int q asm("a");
	return q;
}
