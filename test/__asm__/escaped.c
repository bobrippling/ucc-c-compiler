// RUN: %ucc -S -o- %s
// RUN: %check %s -fsyntax-only

// may error, may not - depends on the assembler. hence -S/-fsyntax-only

x __asm("0new\nli\rne"); // CHECK: /warning: asm name contains character 0xa/
__asm("hi\nthere");
