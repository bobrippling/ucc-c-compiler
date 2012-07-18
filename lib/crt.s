.text
.globl _start
_start:
	# need to set up stack base pointer
	movq %rsp, %rbp

	# rdi = argc
	# rsi = argv
	# rdx = env
	# rcx = __progname
	# rax - used for assigning to bss

	movq (%rsp), %rdi   # argc
	leaq 8(%rsp), %rsi  # argv (before the stackp is altered)

	# __progname = argv[0]
	movq (%rsi), %rcx

	movq %rcx, %rax
	movq %rax, __progname

	# find the first env variable
	leaq 1(%rdi), %rax   # argc + 1

	# rax = argv + (argc + 1) * 8
	leaq (,%rsi,8), %rax  # [%rsi + rax * 8]

	movq %rax, (environ)

	push %rax # environ
	push %rsi # argv
	push %rdi # argc

	call main
	push %rax
	call exit
	hlt

.bss
	# other things we sort out at startup
.globl environ
#environ resq 1
.comm environ,4,4 # ARF??????????

.globl __progname
.comm __progname,4,4 # ARF??????????
