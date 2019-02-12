// RUN: %ucc -S -o- %s

// may error, may not - depends on the assembler

x __asm("0new\nli\rne");
__asm("hi\nthere");
