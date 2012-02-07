section .bss
	global errno
	errno resq 1

section .text
	global __syscall
__syscall:
	push rbp
	mov rbp, rsp

	; x64 convention: rdi, rsi, rdx, rcx, r8, r9
	; linux kernel:   rdi, rsi, rdx, r10, r8, r9
	; rcx and r11 are destroyed - should save...

	mov rax, [rbp + 16] ; eax
	mov rdi, [rbp + 24] ; ebx
	mov rsi, [rbp + 32] ; ecx
	mov rdx, [rbp + 40] ; edx
	mov r10, [rbp + 48] ; edi
	mov r8,  [rbp + 56] ; esi
	mov r9,  [rbp + 64] ; e8?
	syscall
#include "syscall_err.s"
	mov [qword errno], rax
	mov rax, -1
.fin:
	leave
	ret
