section .text
	global memcpy

memcpy:
	; n
	; src
	; dest
	; retaddr
	mov rdi, [rsp + 8]  ; dest
	mov rsi, [rsp + 16] ; src
	mov rcx, [rsp + 24] ; n
	cld
	repnz movsb
	mov rax, [rsp + 24]
	ret
