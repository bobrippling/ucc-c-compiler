present: cmpq %rbx, %rax
present: movl $0, %eax
present: sete %al
present: movl %eax, %edi
present: callq f
