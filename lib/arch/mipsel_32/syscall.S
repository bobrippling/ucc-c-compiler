#include "../../../src/as_cfg.h"
.section SECTION_BSS
.globl errno
errno:
	.long 0 # long?

.section SECTION_TEXT
	.globl __syscall
__syscall:
	# TODO
.long 0 # crash

	syscall

#ifdef __linux__
	jlt $v0, 0, .fin
	neg $v0 # get positive errno
#else
	jnc .fin # MIPS???
#endif

	sw $v0, errno
	li $v0, -1
.fin:
	j $ra
