#include "../defs.h"
.section SECTION_NAME_BSS
	.globl SYMBL(errno)
SYMBL(errno):
	.long 0

.section SECTION_NAME_TEXT
	.globl SYMBL(__syscall)
SYMBL(__syscall):
#if defined(UCC_OS_LINUX)
	push %ebp

	// callee saves:
	push %edi
	push %esi
	push %ebx

	mov ((4 + 1) * 4)(%esp), %eax
	mov ((4 + 2) * 4)(%esp), %ebx
	mov ((4 + 3) * 4)(%esp), %ecx
	mov ((4 + 4) * 4)(%esp), %edx
	mov ((4 + 5) * 4)(%esp), %esi
	mov ((4 + 6) * 4)(%esp), %edi
	mov ((4 + 6) * 4)(%esp), %ebp

#elif defined(UCC_OS_DARWIN)
	/* arguments are on the stack like so:
	 *
	 * ...
	 * [arg3]
	 * [arg2]
	 * [arg1]
	 * [syscall no]
	 * [return address]
	 *
	 * bsd/darwin expect one dummy member before the syscall stack,
	 * so we want this:
	 * ...
	 * [arg3]
	 * [arg2]
	 * [arg1]
	 * [return address]
	 *
	 * and eax = syscall no
	 */

	pop %ecx // return
	pop %eax // syscall no
	push %ecx
#endif

	int $0x80

#if defined(UCC_OS_LINUX)
	cmp $-1, %eax
	jg .fin
	neg %eax # get positive errno
#elif defined(UCC_OS_DARWIN)
	jnc .fin
	// carry - err is in eax
#endif
	movl %eax, SYMBL(errno)
	mov $-1, %eax

.fin:
#if defined(UCC_OS_DARWIN)
	// need to restore the stack
	pop %ecx // return
	push %ecx // dummy
	push %ecx // restore return addr
#else
	// callee saves:
	pop %ebx
	pop %esi
	pop %edi
	pop %ebp
#endif
	ret
