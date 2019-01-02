// RUN: %ucc -o %t %s
// RUN: %ocheck 2 %t
// RUN: %asmcheck %s

__asm(".globl a");
__asm("a: .long 2");

main()
{
	extern int q __asm("a");
	return q;
}
