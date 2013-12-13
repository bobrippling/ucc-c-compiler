// RUN: %ucc -S -o- %s
// RUN: %check -e %s
x asm("0new\nli\rne"); // CHECK: /warning: asm name contains character 0xa/
asm("hi\nthere");
