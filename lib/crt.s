;%define MAC 1
%ifdef MAC
%define SYS_exit  0x2000001
%define SYS_read  0x2000003
%define SYS_write 0x2000004
%else
%define SYS_read  0
%define SYS_write 1
%define SYS_exit  60
%endif

section .text
	extern main

	global exit
exit:
	mov rax,  SYS_exit
	pop rdi
	pop rdi
	syscall
	hlt

	global read
read:
	push rbp
	mov rbp, rsp
	mov rax, SYS_read
	mov rdi, [rbp + 32]
	mov rsi, [rbp + 24]
	mov rdx, [rbp + 16]
	syscall
	leave
	ret

	global write
write:
	push rbp
	mov rbp, rsp
	mov rax, SYS_write
	mov rdi, [rbp + 32]
	mov rsi, [rbp + 24]
	mov rdx, [rbp + 16]
	syscall
	leave
	ret
