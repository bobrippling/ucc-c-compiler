#include "../src/as_cfg.h"
.section SECTION_TEXT
.globl _start
_start:
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

	movq %rcx, __progname(%rip)

	// find the first env variable

	leaq 1(%rdi), %rdx    // rdx = argc + 1
	sal $3, %rdx          // rdx *= 8
	addq %rsi, %rdx       // rdx += argv

	// rdx = (argc + 1) * 8 + argv
	// env = &argv[argc+1]

	movq %rdx, environ(%rip)

	call main
	movl %eax, %edi
	call exit
	hlt

.section SECTION_BSS
// other things we sort out at startup

.globl environ
.globl __progname

// size, align
.comm   environ,  8, 8
.comm __progname, 8, 8
