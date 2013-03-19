.section .data
.globl a
.align 4
a:
.long 2
.byte 7
.byte 7
.space 1 # null scalar init
.space 1 # struct padding
.space 4 # null scalar init
.space 8 # empty array
.space 4 # null scalar init
.space 8 # empty array
.space 4 # null scalar init
.space 8 # empty array
.long 1
.space 8 # empty array
.long 1
.space 8 # empty array
.long 1

.space 8 # empty array

.space 4 # null scalar init
.space 8 # empty array
.space 4 # null scalar init
.space 8 # empty array
.space 4 # null scalar init
.space 8 # empty array
.space 4 # null scalar init
.space 8 # empty array

.long 2
.byte 7
.byte 7
.space 1 # null scalar init
.space 1 # struct padding

.space 4 # null scalar init
.space 8 # empty array
.space 4 # null scalar init
.space 8 # empty array
.space 4 # null scalar init
.space 8 # empty array

.long 1
.space 8 # empty array
.long 1
.space 8 # empty array
.long 1
.space 8 # empty array

.space 4 # null scalar init
.space 8 # empty array
.space 4 # null scalar init
.space 8 # empty array
.space 4 # null scalar init
.space 8 # empty array
.space 4 # null scalar init
.space 8 # empty array

.long 2
.byte 7
.byte 7
.space 1 # null scalar init
.space 1 # struct padding

.space 4 # null scalar init
.space 8 # empty array
.space 4 # null scalar init
.space 8 # empty array
.space 4 # null scalar init
.space 8 # empty array

.long 1
.space 8 # empty array
.long 1
.space 8 # empty array
.long 1
.space 8 # empty array

.space 4 # null scalar init
.space 8 # empty array
.space 4 # null scalar init
.space 8 # empty array
.space 4 # null scalar init
.space 8 # empty array
.space 4 # null scalar init
.space 8 # empty array

.long 2
.byte 7
.byte 7
.space 1 # null scalar init
.space 1 # struct padding

.space 4 # null scalar init
.space 8 # empty array
.space 4 # null scalar init
.space 8 # empty array
.space 4 # null scalar init
.space 8 # empty array

.long 1
.space 8 # empty array
.long 1
.space 8 # empty array
.long 1
.space 8 # empty array

.space 4 # null scalar init
.space 8 # empty array
.space 4 # null scalar init
.space 8 # empty array
.space 4 # null scalar init
.space 8 # empty array
.space 4 # null scalar init
.space 8 # empty array

.space 4 # null scalar init
.space 3 # empty array
.space 1 # struct padding
.space 120 # empty array
.space 4 # null scalar init
.space 3 # empty array
.space 1 # struct padding
.space 120 # empty array
.space 4 # null scalar init
.space 3 # empty array
.space 1 # struct padding
.space 120 # empty array
.space 4 # null scalar init
.space 3 # empty array
.space 1 # struct padding
.space 120 # empty array

# FIXME
# missing: .zero 8, 36, 4
# perhaps missing a starting-struct init?

.long 2
.byte 3

.space 1 # null scalar init
.space 1 # null scalar init
.space 1 # struct padding
.space 120 # empty array
