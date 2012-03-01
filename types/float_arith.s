section .data
third dq    0.333333333
five  dq    5.0

section .bss
flt  resq  1

section .text
	global _start
_start:
lit5:
	fld  qword [five] ; load
	fstp qword [flt]    ; store

	fld qword [flt]
	sub rsp, 8
	fstp qword [rsp]
	;call printf
	add rsp, 8

;addb:                ; c=a+b;
;	fld    qword [a]     ; load a (pushed on flt pt stack, st0)
;	fadd    qword [b]    ; floating add b (to st0)
;	fstp    qword [c]    ; store into c (pop flt pt stack)
;	pabc    "c=a+b"
;
;subb:                ; c=a-b;
;	fld    qword [a]     ; load a (pushed on flt pt stack, st0)
;	fsub    qword [b]    ; floating subtract b (to st0)
;	fstp    qword [c]    ; store into c (pop flt pt stack)
;	pabc    "c=a-b"
;
;mulb:                ; c=a*b;
;	fld    qword [a]    ; load a (pushed on flt pt stack, st0)
;	fmul    qword [b]    ; floating multiply by b (to st0)
;	fstp    qword [c]    ; store product into c (pop flt pt stack)
;	pabc    "c=a*b"
;
;diva:                ; c=c/a;
;	fld    qword [c]     ; load c (pushed on flt pt stack, st0)
;	fdiv    qword [a]    ; floating divide by a (to st0)
;	fstp    qword [c]    ; store quotient into c (pop flt pt stack)
;	pabc    "c=c/a"

	mov rax, 0x2000000
	mov rdi, 5
	syscall
	hlt
