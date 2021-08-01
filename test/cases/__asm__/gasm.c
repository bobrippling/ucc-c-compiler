// RUN: %ocheck 2 %s
// RUN: %ucc -target x86_64-linux -S -o- %s | %stdoutcheck %s

__asm(".globl a");   // STDOUT: .globl a
__asm("a: .long 2"); // STDOUT: a: .long 2

main()
{
#include "../ocheck-init.c"
	extern int q __asm("a");

	// STDOUT: /.globl _?main/
	// STDOUT-NEVER: /mov.* q/
	// STDOUT: /mov.* a/

	return q;
}
