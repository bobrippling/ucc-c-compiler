section .text
global main
main:
	push rbp
	mov rbp, rsp
	mov eax, 4 ; ???
	push rax
	mov eax, 2 ; ???
	push rax
	xor rdx, rdx
	pop rbx
	pop rax
	idiv eax
	push rax
	pop rax
	; return
	jmp .main.1
.main.1:
	leave
	ret
section .data
section .bss
