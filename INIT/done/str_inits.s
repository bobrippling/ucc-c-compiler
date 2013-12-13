.section __TEXT,__text
.globl p
.globl s
.globl t
.section __DATA,__data
.align 8
p:
.quad str.1

.align 1
s:
.byte 104
.byte 101
.byte 108
.byte 108
.byte 111
.byte 0

.align 1
t:
.byte 104
.byte 101
.byte 108
.byte 108
.byte 111
.byte 0

.align 1
str.1:
.byte 104
.byte 101
.byte 108
.byte 108
.byte 111
.byte 0

.align 1
str.2:
.byte 104
.byte 101
.byte 108
.byte 108
.byte 111
.byte 0

.align 1
str.3:
.byte 104
.byte 101
.byte 108
.byte 108
.byte 111
.byte 0

.section __BSS,__bss
