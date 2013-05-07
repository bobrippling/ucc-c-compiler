#include "../../../src/as_cfg.h"
#include "defs.h"
.section SECTION_TEXT
.globl SYMBL(_start)
SYMBL(_start):
	// need to set up stack base pointer
	movq %rsp, %rbp

	// rdi = argc
	// rsi = argv
	// rdx = env
	// rcx = __progname
	// rax - used for assigning to bss

	movq (%rsp), %rdi   // argc
	leaq 8(%rsp), %rsi  // argv (before the stackp is altered)

	// __progname = argv[0]
	movq (%rsi), %rcx

	movq %rcx, SYMBL(__progname)(%rip)

	// find the first env variable

	leaq 1(%rdi), %rdx    // rdx = argc + 1
	// rax = argv + (argc + 1) * 8

	leaq (%rsi, %rdx, 8), %rdx // argv + new_argc * 8

	movq %rdx, SYMBL(environ)(%rip)

	// argc=rdi, argv=rsi, environ=rdx - FIXME: this is only Linux ABI
	call SYMBL(main)
	movl %eax, %edi
	call SYMBL(exit)
	hlt

.section SECTION_BSS
// other things we sort out at startup

.globl SYMBL(environ)
.globl SYMBL(__progname)

// size, align
.comm SYMBL(  environ),  8, 8
.comm SYMBL(__progname), 8, 8
