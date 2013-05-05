// RUN: echo TODO; false

// test of ABI
// compile: gcc -c -O2
// use objdump -d

int f(int a, int b, int c, int d, int e, int f, int g, int h) {
    return a+b+c+d+e+f+g+h;
}

double g(double a, double b, double c, double d, double e, double f, double g, double h) {
    return a+b+c+d+e+f+g+h;
}

// test
int main() {
    int i = f(1,2,3,4,5,6,7,8);
    double x = g(1,2,3,4,5,6,7,8);
    return i+x;
}

//==================================================================
//abi-test.o:     file format elf64-x86-64
//Disassembly of section .text:
//
//0000000000000000 <f>:
//   0: 8d 3c 3e                lea    (%rsi,%rdi,1),%edi
//   3: 8d 14 17                lea    (%rdi,%rdx,1),%edx
//   6: 01 ca                   add    %ecx,%edx
//   8: 44 01 c2                add    %r8d,%edx
//   b: 42 8d 04 0a             lea    (%rdx,%r9,1),%eax
//   f: 03 44 24 08             add    0x8(%rsp),%eax
//  13: 03 44 24 10             add    0x10(%rsp),%eax
//  17: c3                      retq
//  18: 0f 1f 84 00 00 00 00    nopl   0x0(%rax,%rax,1) //padding
//  1f: 00
//
//0000000000000020 <g>:
//  20: f2 0f 58 c1             addsd  %xmm1,%xmm0
//  24: f2 0f 58 c2             addsd  %xmm2,%xmm0
//  28: f2 0f 58 c3             addsd  %xmm3,%xmm0
//  2c: f2 0f 58 c4             addsd  %xmm4,%xmm0
//  30: f2 0f 58 c5             addsd  %xmm5,%xmm0
//  34: f2 0f 58 c6             addsd  %xmm6,%xmm0
//  38: f2 0f 58 c7             addsd  %xmm7,%xmm0
//  3c: c3                      retq
//  3d: 0f 1f 00                nopl   (%rax)
//
//0000000000000040 <main>:
//  40: b8 48 00 00 00          mov    $0x48,%eax       // optimized
//  45: c3                      retq

//==================================================================
//abi-test.o:     file format elf32-i386
//Disassembly of section .text:
//
//00000000 <f>:
//   0: 55                      push   %ebp
//   1: 89 e5                   mov    %esp,%ebp
//   3: 8b 45 0c                mov    0xc(%ebp),%eax
//   6: 03 45 08                add    0x8(%ebp),%eax
//   9: 03 45 10                add    0x10(%ebp),%eax
//   c: 03 45 14                add    0x14(%ebp),%eax
//   f: 03 45 18                add    0x18(%ebp),%eax
//  12: 03 45 1c                add    0x1c(%ebp),%eax
//  15: 03 45 20                add    0x20(%ebp),%eax
//  18: 03 45 24                add    0x24(%ebp),%eax
//  1b: 5d                      pop    %ebp
//  1c: c3                      ret
//  1d: 8d 76 00                lea    0x0(%esi),%esi
//
//00000020 <g>:
//  20: 55                      push   %ebp
//  21: 89 e5                   mov    %esp,%ebp
//  23: dd 45 10                fldl   0x10(%ebp)
//  26: dc 45 08                faddl  0x8(%ebp)
//  29: dc 45 18                faddl  0x18(%ebp)
//  2c: dc 45 20                faddl  0x20(%ebp)
//  2f: dc 45 28                faddl  0x28(%ebp)
//  32: dc 45 30                faddl  0x30(%ebp)
//  35: dc 45 38                faddl  0x38(%ebp)
//  38: dc 45 40                faddl  0x40(%ebp)
//  3b: 5d                      pop    %ebp
//  3c: c3                      ret
//  3d: 8d 76 00                lea    0x0(%esi),%esi
//
//00000040 <main>:
//  40: 55                      push   %ebp
//  41: b8 48 00 00 00          mov    $0x48,%eax
//  46: 89 e5                   mov    %esp,%ebp
//  48: 5d                      pop    %ebp
//  49: c3                      ret
//==================================================================
