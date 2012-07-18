.text
.globl _start
_start:
	# need to set up stack base pointer
	movq %rsp, %rbp

	call main
	mov %rax, %rdi
	mov $60, %rax
	syscall
	hlt
