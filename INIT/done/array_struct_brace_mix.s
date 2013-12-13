.section __TEXT,__text
.globl x
.section __DATA,__data
.align 4
x:
.long 1
.space 4 # null scalar init
.space 4 # null scalar init
.space 4 # null scalar init
.space 12 # empty array
.space 4 # null scalar init
.long 2
.long 3
.long 4
.long 5
.long 6
.long 7
.long 8
.long 9
.space 4 # null scalar init
.space 4 # null scalar init
.space 4 # null scalar init
.space 4 # null scalar init
.space 12 # empty array
.space 4 # null scalar init
.long 8
.long 9
.long 10
.long 1
.long 11
.long 12
.long 13
.long 5
.long 14
.space 4 # null scalar init
.space 4 # null scalar init
.space 4 # null scalar init
.space 12 # empty array
.space 4 # null scalar init

.section __BSS,__bss
