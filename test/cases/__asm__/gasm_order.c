// RUN: %ucc -c %s
// RUN: %ucc -S -o- %s | tr -d '\n' | grep '.globl Z.*A.*B.*C.*D.*E.*F'

__asm__(".globl Z");
A(){}
__asm(".globl B");
C(){}
__asm(".globl D");
E(){}
__asm__(".globl F");
