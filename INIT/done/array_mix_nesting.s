.section __TEXT,__text
.globl g
.section __DATA,__data
.align 4
g:
.long 1
.space 4 # null scalar init
.long 2
.long 3
.long 4
.space 4 # null scalar init
.long 5
.long 6
.long 7
.space 4 # null scalar init

.section __BSS,__bss
