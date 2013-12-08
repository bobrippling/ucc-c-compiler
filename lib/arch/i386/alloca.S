#include "../../../src/as_cfg.h"
.section SECTION_TEXT
.globl alloca
alloca:
	pop %esi // return addr

	// FIXME: need to align to stack boundary
	subl %esi, %esp
	movl %esp, %eax

	jmp *%esi
