#include "../defs.h"
.section SECTION_NAME_TEXT
.globl alloca
alloca:
	popq %rsi // return addr

	// FIXME: need to align to stack boundary
	subq %rdi, %rsp // rdi = amount to alloc
	movq %rsp, %rax // return

	jmp *%rsi
