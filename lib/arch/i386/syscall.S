#include "../../../src/as_cfg.h"
#include "../defs.h"
.section SECTION_BSS
	.globl SYMBL(errno)
SYMBL(errno):
	.long 0

.section SECTION_TEXT
	.globl SYMBL(__syscall)
SYMBL(__syscall):
	push %ebp
	mov  %esp, %ebp

	// %eax for syscall num,
	// then %ebx, %ecx, %edx, %esi, %edi, %ebp

	//  0(ebp) = saved ebp
	//  4(ebp) = ret addr
	//  8(ebp) = arg0
	// 12(ebp) = arg1
	// ...

	movl  8(%ebp), %ebx
	movl 12(%ebp), %ecx
	movl 16(%ebp), %edx
	movl 20(%ebp), %esi
	movl 24(%ebp), %edi
	movl 28(%ebp), %ebp

	// if >6 params, ebx points to the rest... TODO

	int $0x80

#ifdef __linux__
	cmpl $-1, %eax
	jg .fin
	negl %eax # get positive errno
#else
	jnc .fin
	// carry - err is in rax
#endif
	movl %eax, SYMBL(errno)
	movl $-1, %eax
.fin:

	mov %ebp, %esp
	pop %ebp
	ret
