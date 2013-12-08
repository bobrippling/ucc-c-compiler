#include "../../../src/as_cfg.h"
#include "../defs.h"
.section SECTION_TEXT
// start is special, since it's an asm-only symbol

#ifdef __LEADING_UNDERSCORE
.globl start
start:
#else
.globl _start
_start:
#endif

	xorq %rbp, %rbp

	popq %rsi
	movq %rsp, %rdx
	leaq SYMBL(main)(%rip), %rdi

	// 1: main, 2: argc, 3: argv
	call SYMBL(__libc_start_main)
.Ll:
	hlt
	jmp .Ll
