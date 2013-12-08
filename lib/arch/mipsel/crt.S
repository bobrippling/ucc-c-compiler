#include "../../../src/as_cfg.h"
.section SECTION_TEXT
.globl _start
_start:
	move $fp, $sp

	TODO: # argc, argv, environ, __progname

	jal main
	move $a0, $v0
	j exit

.section SECTION_BSS
.globl environ
.comm  environ, 8, 8 # size, align

.globl __progname
.comm  __progname, 8, 8
