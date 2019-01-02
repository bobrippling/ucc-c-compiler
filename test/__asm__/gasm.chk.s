present: .globl a
present: a: .long 2
present: /.globl _?main/
present: /mov.* a/
absent: /mov.* q/
