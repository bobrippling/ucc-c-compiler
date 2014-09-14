// RUN: %ucc -S -o- %s
// RUN: %check -e %s
x __asm("0new\nli\rne"); // CHECK: /warning: asm name contains character 0xa/
__asm("hi\nthere");
