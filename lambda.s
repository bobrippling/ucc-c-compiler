.data
L1: .ascii "%d\n\0"
L2: .ascii "lambda tramp @ %p\n\0"

.text
.globl main

main:
	push %rbp
	mov %rsp, %rbp
	sub $48, %rsp

	movl $3, -4(%rbp)

	// create lambda
	// -16 = add lambda

	// -32 = lambda trampoline
	// -40 = "
	// -48 = "
	lea -48(%rbp), %rax
	mov %rax, -16(%rbp)

	movw $0xb948, -48(%rbp) /* movabs [...], %rcx */
	mov %rbp, -46(%rbp)

	movw $0xba49, -38(%rbp) /* movabs [...], %r10 */
	lea add(%rip), %rax
	mov %rax, -36(%rbp)

	movl $0xe2ff41, -28(%rbp) /* jmpq *%r10 */

	lea L2(%rip), %rdi
	mov -16(%rbp), %rsi
	mov $0, %al
	call printf

	// .!tr ' ' '\n'|tac|tr '\n' ' '
// 49 bb _6a 0d 00 00 01 00 00 00 movabs $0x100000d6a,%r11
// 49 ba _24 f7 bf 5f ff 7f 00 00 movabs $0x7fff5fbff724,%r10
// 41 ff e3                jmpq   *%r11

	mov -16(%rbp), %rdi
	mov $5, %rsi
	call indirect

	lea L1(%rip), %rdi
	mov %rax, %rsi
	mov $0, %al
	call printf

	movl $0, %eax

	leave
	ret

indirect:
	push %rbp
	mov %rsp, %rbp

	mov %rdi, %rax
	mov %rsi, %rdi
	call *%rax

	leave
	ret


// add(int a) -- %rcx contains context
add:
	push %rbp
	mov %rsp, %rbp

	mov -4(%rcx), %ecx
	add %edi, %ecx
	mov %ecx, %eax

	leave
	ret
