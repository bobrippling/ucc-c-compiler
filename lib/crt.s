section .text
	extern main
	extern exit

	global _start
_start:
	; need to set up stack base pointer
	mov rbp, rsp

	; rdi = argc
	; rsi = argv
	; rdx = env
	; rcx = __progname
	; rax - used for assigning to bss

	mov rdi, [rsp]     ; argc
	lea rsi, [rsp + 8] ; argv (before the stackp is altered)

	; __progname = argv[0]
	mov rcx, [rsi]

	mov rax, rcx
	mov [qword __progname], rax

	; find the first env variable
	lea rax, [rdi + 1]       ; argc + 1
	lea rax, [rsi + rax * 8] ; rax = argv + (argc + 1) * 8
	mov [qword environ], rax

	push rax ; environ
	push rsi ; argv
	push rdi ; argc

	call main
	push rax
	call exit
	hlt

section .bss
	; other things we sort out at startup
	global environ
	environ resq 1

	global __progname
	__progname resq 1
