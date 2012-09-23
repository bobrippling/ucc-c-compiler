section .text
global f
f:
	push rbp
	mov rbp, rsp
	mov eax, 3
	push rax
	push rax
	; value to save
	mov rax, [rbp + 16]   ; FIXME: this should be mov Eax, [rbp + 16]
	; i
	push rax
	; pointer on stack
	pop rax
	; address
	pop rbx
	; value
	mov [rax + 0], rbx    ; FIXME: this should be mov [rax + 0], Ebx
	pop rax
	; unused expr
.f.1:
	leave
	ret
global main
main:
	push rbp
	mov rbp, rsp
	sub rsp, 8
	lea rax, [rbp + -8]
	; &i
	push rax
	call f
	add rsp, 8
	; 1 arg
	push rax
	pop rax
	; unused expr
	mov eax, [rbp + -8]
	; i
	push rax
	pop rax
	; return
	jmp .main.2
.main.2:
	add rsp, 8
	leave
	ret
section .data
section .bss
