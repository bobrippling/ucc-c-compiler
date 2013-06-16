.section __TEXT,__text
.globl x
.globl y
.globl z
.section __DATA,__data
.align 4
x:
.space 8 # empty array
.space 4 # null scalar init
.long 1
.long 2
.space 4 # null scalar init
.space 4 # null scalar init
.long 3
.long 1
.long 2
.long 2
.space 4 # null scalar init

.align 4
y:
.space 8 # empty array
.space 4 # null scalar init
.long 1

.align 4
z:
.space 8 # empty array
.space 4 # null scalar init
.long 1

.section __BSS,__bss
