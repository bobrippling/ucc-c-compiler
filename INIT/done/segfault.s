.section __TEXT,__text
.globl game
.globl game2
.section __DATA,__data
.align 4
game:
.long 1
.space 4 # null scalar init
.space 4 # null scalar init
.space 4 # null scalar init
.space 4 # null scalar init
.space 4 # null scalar init
.long 2
.space 4 # null scalar init
.space 4 # null scalar init
.space 4 # null scalar init
.space 4 # null scalar init
.space 4 # null scalar init

.align 4
game2:
.long 1
.space 4 # null scalar init
.space 4 # null scalar init
.space 4 # null scalar init
.space 4 # null scalar init
.space 4 # null scalar init
.long 2
.space 4 # null scalar init
.space 4 # null scalar init
.space 4 # null scalar init
.space 4 # null scalar init
.space 4 # null scalar init

.section __BSS,__bss
