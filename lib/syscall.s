section .bss
	errno resq 1

section .text
	global __syscall
__syscall:
	push rbp
	mov rbp, rsp
	mov rax, [rbp + 40]
	mov rdi, [rbp + 32]
	mov rsi, [rbp + 24]
	mov rdx, [rbp + 16]
	syscall
	cmp rax, -1
	je .err
	jmp .fin
.err:
	neg rax
	mov [errno], rax
	mov rax, -1
.fin:
	leave
	ret
