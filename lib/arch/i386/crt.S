#include "../../../src/as_cfg.h"
#include "../defs.h"
.section SECTION_TEXT

#ifdef __LEADING_UNDERSCORE
.globl start
start:
#else
.globl _start
_start:
#endif

	xor %ebp, %ebp

	popl %esi
	movl %esp, %edx
	leal SYMBL(main), %edi

	// 1: main, 2: argc, 3: argv
	call SYMBL(__libc_start_main)
.Ll:
	hlt
	jmp .Ll
