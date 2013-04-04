// RUN: %ucc -c %s
// RUN: %ucc -S -o- %s | tr -d '\n' | grep '.globl 0.*A.*B.*C.*D.*E.*F'

__asm__(".globl 0");
A(){}
asm(".globl B");
C(){}
__asm(".globl D");
E(){}
__asm__(".globl F");
