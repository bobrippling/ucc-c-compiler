section .text
	global _start
	global write
	global exit
	extern main

_start:
	pop rdi ; argc
	mov rsi, rsp ; argv
	push rdi
	push rsi
	call main
	mov rax, 0
	push rax
	call exit

exit:
	mov rax, 60
	pop rdi
	pop rdi
	syscall
	hlt

write:
	push rbp
	mov rbp, rsp
	mov rax, 1
	mov rdi, [rbp + 16]
	mov rsi, [rbp + 24]
	mov rdx, [rbp + 32]
	syscall
	leave
	ret
