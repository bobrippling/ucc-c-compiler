// RUN: %ucc -S -o- %s

// may error, may not - depends on the assembler

x __asm("0new\nli\rne"); // CHECK: /warning: asm name contains character 0xa/
__asm("hi\nthere");
