section .bss
	errno resq 1

section .text
	global __syscall
__syscall:
	push rbp
	mov rbp, rsp
	mov rax, [rbp + 16]
	mov rdi, [rbp + 24]
	mov rsi, [rbp + 32]
	mov rdx, [rbp + 40]
	syscall
	cmp rax, -1
	je .err
	jmp .fin
.err:
	neg rax
	mov [qword errno], rax
	mov rax, -1
.fin:
	leave
	ret
