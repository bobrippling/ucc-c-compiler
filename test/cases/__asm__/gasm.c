// RUN: %ocheck 2 %s
// RUN: %ucc -target x86_64-linux -S -o %t %s
// RUN: grep -F '.globl a' %t
// RUN: grep -F 'a: .long 2' %t
// RUN: grep '.globl _\?main' %t
// RUN: grep 'mov.* a' %t
// RUN: ! grep 'mov.* q' %t

__asm(".globl a");
__asm("a: .long 2");

main()
{
	extern int q __asm("a");
	return q;
}
