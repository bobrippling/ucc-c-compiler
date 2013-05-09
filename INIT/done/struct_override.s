.section __TEXT,__text
.globl x
.globl y
.section __DATA,__data
.align 4
x:
.byte 113
.byte 98
.byte 99
.byte 0
.space 1 # null scalar init
.space 1 # null scalar init
.space 2 # struct padding
.long 1

.align 4
y:
.byte 113
.byte 98
.byte 99
.byte 0
.space 1 # null scalar init
.space 1 # null scalar init
.space 2 # struct padding
.long 1

.align 1
str.1:
.byte 97
.byte 98
.byte 99
.byte 0

.align 1
str.2:
.byte 97
.byte 98
.byte 99
.byte 0

.section __BSS,__bss
