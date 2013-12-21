.section .data
msg:
.long 0

caption:
.byte 'h', 'i', 0

happyface:
.byte ':', ')', 0

sadface:
.byte ':', '(', 0

.section .text
_start:
	calll main

	// _MessageBoxA
	pushl $0
	pushl $caption
	pushl msg
	pushl $0
	calll _MessageBoxA

	pushl $36
	calll _ExitProcess
	hlt

main:
	pushl $5687
	pushl $1
	pushl $2
	calll f
	popl %eax
	cmpl $5687, %eax
	jne L.sadface
	movl $happyface, msg
	jmp L.fin
L.sadface:
	movl $sadface, msg
L.fin:
	ret

// stdcall
f:
	ret $8
