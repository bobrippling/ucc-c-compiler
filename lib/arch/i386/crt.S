#include "../defs.h"
.section SECTION_NAME_TEXT
// start is special, since it's an asm-only symbol

#ifdef __LEADING_UNDERSCORE
.globl start
start:
#else
.globl _start
_start:
#endif

	xor %ebp, %ebp

	popl %ecx // argc
	mov %esp, %edx // argv
	lea SYMBL(main), %eax

	pushl %edx
	pushl %ecx
	pushl %eax

	// 1: main, 2: argc, 3: argv
	call SYMBL(__libc_start_main)
.Ll:
	hlt
	jmp .Ll
