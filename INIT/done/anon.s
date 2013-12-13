.section __TEXT,__text
.globl a
.globl b
.globl c
.globl d
.globl e
.section __DATA,__data
.align 4
a:
.long 0
.long 1
.space 4 # null scalar init
.long 2

.align 4
b:
.space 4 # null scalar init
.space 4 # null scalar init
.long 2
.long 3

.align 4
c:
.long 1
.long 2
.long 3
.space 4 # null scalar init

.align 4
d:
.space 4 # null scalar init
.space 4 # null scalar init
.space 4 # null scalar init
.long 1

.align 4
e:
.long 1
.long 2
.long 3
.space 4 # null scalar init

.section __BSS,__bss
