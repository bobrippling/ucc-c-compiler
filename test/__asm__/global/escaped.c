// RUN: %check --only %s -fsyntax-only

// may error, may not - depends on the assembler. hence -fsyntax-only (%check)

x __asm("0new\nli\rne"); // CHECK: warning: asm name contains character 0xa
__asm("hi\nthere");
