// RUN: %check --only -e %s

__asm__ volatile (""); // CHECK: Qualification after global asm (volatile)
__asm__ const (""); // CHECK: Qualification after global asm (const)
__asm__ __const (""); // CHECK: Qualification after global asm (const)
