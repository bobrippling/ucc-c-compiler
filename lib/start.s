section .text
	extern exit
	extern main
	global _start

_start:
	; need to set up stack base pointer
	mov rbp, rsp

	; push argc, then argv (address of the stack)

	lea rbx, [rsp + 8] ; argv (before the stackp is altered)
	mov rax, [rsp]     ; argc

	push rax ; argc
	push rbx ; argv

	call main
	push rax
	call exit
	hlt
