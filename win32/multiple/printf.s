.globl printf

caption:
.byte 'c', 'a', 'p', 't', 'i', 'o', 'n', 0

printf:
	movl 4(%esp), %eax

	pushl $0
	pushl $caption
	pushl %eax
	pushl $0

	// WINAPI, aka stdcall
	calll _MessageBoxA
	ret
