section .text
	extern main
	extern exit

	global _start
_start:
	; need to set up stack base pointer
	mov rbp, rsp

	; push argc, then argv (address of the stack)

	lea rbx, [rsp + 8] ; argv (before the stackp is altered)
	mov rax, [rsp]     ; argc

	; argv[0]
	mov rcx, [rbx]
	mov [__progname], rcx

	; find the first env variable
	mov rcx, rbx ; argv
	inc rax
	lea rcx, [rcx + rax * 8] ; rcx = argv + argc + 1
	dec rax
	mov [environ], rcx

	push rcx ; environ
	push rbx ; argv
	push rax ; argc

	call main
	push rax
	call exit
	hlt

section .data
	; other things we sort out at startup
	global environ
	environ dq 1

	global __progname
	__progname dq 1
