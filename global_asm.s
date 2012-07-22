.section .text, ""
#.globl i
.globl _main
_main:
	pushq %rbp
	movq %rsp, %rbp
	movl $5, %eax
	pushq %rax
	movl %eax, i@GOTPCREL(%rip)
	# i
	popq %rax
	# unused expr
	movl i@GOTPCREL(%rip), %eax
	# i
	pushq %rax
	popq %rax
	# return
	jmp .main.1
.main.1:
	leave
	ret

.section .data,
.comm i, 4

#.section .data, ""
#.section .bss, ""
#i:
#.long 0
