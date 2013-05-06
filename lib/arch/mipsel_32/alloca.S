#include "../../../src/as_cfg.h"
.section SECTION_TEXT
.globl alloca
alloca:
	# FIXME: need to align to stack boundary
	sub $sp, $sp, $a0
	j $ra
