section .text
	global alloca

alloca:
	; stack looks like:

	; vars, etc...  <-- rsp
	; etc...
	; amount_to_alloca
	; return addr (continue here) <-- rbp

	pop rdi ; return addr
	pop rsi ; amount to alloc

	sub rsp, rsi
	mov rax, rsp ; return

	jmp rdi
