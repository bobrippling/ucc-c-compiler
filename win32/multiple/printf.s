.globl printf

caption:
.byte 'h', 'e', 'l', 'l', 'o', 0

printf:
	pushl $0
	pushl $caption
	pushl %edi
	pushl $0
	calll _MessageBoxA
	//addl $16, %esp
	ret
