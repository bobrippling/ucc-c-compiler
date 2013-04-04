// RUN: %ucc -c %s
// RUN: %asmcheck -c %s

__asm__(".globl 0");
A(){}
asm(".globl B");
C(){}
__asm(".globl D");
E(){}
__asm__(".globl F");
